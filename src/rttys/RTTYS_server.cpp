//
// Created by stephane bourque on 2021-11-23.
//

#include "rttys/RTTYS_server.h"
#include "rttys/RTTYS_WebServer.h"
#include "rttys/RTTYS_device.h"
#include "rttys/RTTYS_ClientConnection.h"

#include "framework/MicroServiceFuncs.h"
#include "fmt/format.h"

namespace OpenWifi {

	int RTTYS_server::Start() {

		Internal_ = MicroServiceConfigGetBool("rtty.internal",false);
		if(Internal_) {
			int DSport = (int) MicroServiceConfigGetInt("rtty.port", 5912);
			int CSport = (int) MicroServiceConfigGetInt("rtty.viewport", 5913);
			RTTY_UIAssets_ = MicroServiceConfigPath("rtty.assets", "$OWGW_ROOT/rtty_ui");
			MaxConcurrentSessions_ = MicroServiceConfigGetInt("rtty.maxsessions",0);

			const auto & CertFileName = MicroServiceConfigPath("openwifi.restapi.host.0.cert","");
			const auto & KeyFileName = MicroServiceConfigPath("openwifi.restapi.host.0.key","");
			const auto & RootCa = MicroServiceConfigPath("openwifi.restapi.host.0.rootca","");

			if(MicroServiceNoAPISecurity()) {
				Poco::Net::ServerSocket DeviceSocket(DSport, 64);
				// DeviceAcceptor_ = std::make_unique<Poco::Net::SocketAcceptor<RTTYS_Device_ConnectionHandler>>(DeviceSocket,DeviceReactor_);
			} else {
				auto DeviceSecureContext = Poco::AutoPtr<Poco::Net::Context>( new Poco::Net::Context(Poco::Net::Context::SERVER_USE,
																  KeyFileName, CertFileName, "",
																  Poco::Net::Context::VERIFY_RELAXED));
				Poco::Crypto::X509Certificate DeviceRoot(RootCa);
				DeviceSecureContext->addCertificateAuthority(DeviceRoot);
				DeviceSecureContext->disableStatelessSessionResumption();
				DeviceSecureContext->enableSessionCache();
				DeviceSecureContext->setSessionCacheSize(0);
				DeviceSecureContext->setSessionTimeout(10);
				DeviceSecureContext->enableExtendedCertificateVerification(true);
				SSL_CTX *SSLCtxDevice = DeviceSecureContext->sslContext();
				SSL_CTX_dane_enable(SSLCtxDevice);

				Poco::Net::SecureServerSocket DeviceSocket(DSport, 64, DeviceSecureContext);
				// DeviceAcceptor_ = std::make_unique<Poco::Net::SocketAcceptor<RTTYS_Device_ConnectionHandler>>(DeviceSocket,DeviceReactor_);
				DeviceReactor_.addEventHandler(
					DeviceSocket,
					Poco::NObserver<RTTYS_server, Poco::Net::ReadableNotification>
					(*this, &RTTYS_server::onAccept));
			}
			DeviceReactorThread_.start(DeviceReactor_);
			Utils::SetThreadName(DeviceReactorThread_,"rt:devreactor");

			auto WebServerHttpParams = new Poco::Net::HTTPServerParams;
			WebServerHttpParams->setMaxThreads(50);
			WebServerHttpParams->setMaxQueued(200);
			WebServerHttpParams->setKeepAlive(true);
			WebServerHttpParams->setName("rt:dispatch");

			if(MicroServiceNoAPISecurity()) {
				Poco::Net::ServerSocket ClientSocket(CSport, 64);
				ClientSocket.setNoDelay(true);
				WebServer_ = std::make_unique<Poco::Net::HTTPServer>(new RTTYS_Client_RequestHandlerFactory(Logger()), ClientSocket, WebServerHttpParams);
			} else {
				auto WebClientSecureContext = new Poco::Net::Context(Poco::Net::Context::SERVER_USE, KeyFileName, CertFileName,
										   "", Poco::Net::Context::VERIFY_RELAXED);
				Poco::Crypto::X509Certificate WebRoot(RootCa);
				WebClientSecureContext->addCertificateAuthority(WebRoot);
				WebClientSecureContext->disableStatelessSessionResumption();
				WebClientSecureContext->enableSessionCache();
				WebClientSecureContext->setSessionCacheSize(0);
				WebClientSecureContext->setSessionTimeout(10);
				WebClientSecureContext->enableExtendedCertificateVerification(true);
				SSL_CTX *SSLCtxClient = WebClientSecureContext->sslContext();
				SSL_CTX_dane_enable(SSLCtxClient);

				Poco::Net::SecureServerSocket ClientSocket(CSport, 64, WebClientSecureContext);
				ClientSocket.setNoDelay(true);
				WebServer_ = std::make_unique<Poco::Net::HTTPServer>(new RTTYS_Client_RequestHandlerFactory(Logger()), ClientSocket, WebServerHttpParams);
			};
			WebServer_->start();
			ClientReactorThread_.start(ClientReactor_);
			Utils::SetThreadName(ClientReactorThread_,"rt:clntreactor");
		}


		GCCallBack_ = std::make_unique<Poco::TimerCallback<RTTYS_server>>(*this, &RTTYS_server::onTimer);
		Timer_.setStartInterval(30 * 1000);  // first run in 30 seconds
		Timer_.setPeriodicInterval(5 * 1000);
		Timer_.start(*GCCallBack_, MicroServiceTimerPool() );
		NotificationManager_.start(*this);

		return 0;
	}

	void RTTYS_server::Stop() {
		Timer_.stop();
		if(Internal_) {
			ResponseQueue_.wakeUpAll();
			NotificationManager_.wakeUp();
			NotificationManager_.join();
			WebServer_->stopAll(true);
			WebServer_->stop();
			ClientReactor_.stop();
			ClientReactorThread_.join();
			DeviceReactor_.stop();
			// DeviceAcceptor_->unregisterAcceptor();
			DeviceReactorThread_.join();
			NotificationManagerRunning_ = false;
		}
	}

	void RTTYS_server::onAccept(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
		Poco::Net::SocketAddress	Client;
		Poco::Net::StreamSocket NewSocket = pNf->socket().impl()->acceptConnection(Client);
		auto TID = ++CurrentTID_;
		auto NewDevice = std::make_shared<RTTYS_Device_ConnectionHandler>(NewSocket, DeviceReactor_, TID);
		NotifyDeviceConnection(NewDevice, TID);
	}

	void RTTYS_server::onTimer([[maybe_unused]] Poco::Timer & timer) {
		poco_trace(Logger(),"Removing stale connections.");
		Utils::SetThreadName("rt:janitor");
		static auto LastStats = Utils::Now();

		std::lock_guard 	Lock(LocalMutex_);
 		for(auto element=EndPoints_.begin();element!=EndPoints_.end();) {
			if(element->second->TooOld()) {
				auto c = fmt::format("Removing {}. Serial: {} Device connection time: {}s. Client connection time: {}s",
									 element->first,
									 element->second->SerialNumber(),
									 element->second->TimeDeviceConnected(),
									 element->second->TimeClientConnected());
				Logger().information(c);
				TotalConnectedClientTime_ += element->second->TimeClientConnected();
				TotalConnectedDeviceTime_ += element->second->TimeDeviceConnected();
				element = EndPoints_.erase(element);
			} else {
				++element;
			}
		}
		FailedDevices.clear();
		FailedClients.clear();

		if(Utils::Now()-LastStats>(60*5)) {
			LastStats = Utils::Now();
			Logger().information(fmt::format("Statistics: Total connections:{} Total Device Connection Time: {}s  Total Client Connection Time: {}s Device failures: {} Client failures: {}",
				TotalEndPoints_,
				TotalConnectedDeviceTime_,
				TotalConnectedClientTime_,
				FailedNumDevices_,
				FailedNumClients_));
		}
	}

	void RTTYS_server::CreateNewClient(	Poco::Net::HTTPServerRequest &request,
										Poco::Net::HTTPServerResponse &response,
									   	const std::string &id) {

		auto NewClient = std::make_shared<RTTYS_ClientConnection>(request, response, ClientReactor_, id);
		NotifyClientRegistration(id,NewClient);
	}

	void RTTYS_server::run() {
		Utils::SetThreadName("rt:manager");
		NotificationManagerRunning_ = true;
		Poco::AutoPtr<Poco::Notification> NextNotification(ResponseQueue_.waitDequeueNotification());
		while (NextNotification && NotificationManagerRunning_) {
			auto Notification = dynamic_cast<RTTYS_Notification *>(NextNotification.get());
			if (Notification != nullptr) {
				std::lock_guard 	Lock(LocalMutex_);
				auto It = EndPoints_.find(Notification->id_);
				if (It != EndPoints_.end()) {
					switch (Notification->type_) {
					case RTTYS_Notification_type::device_disconnection: {
						It->second->DisconnectDevice();
					} break;
					case RTTYS_Notification_type::client_disconnection: {
						It->second->DisconnectClient();
					} break;
					case RTTYS_Notification_type::device_registration: {
						auto Device = ConnectingDevices_.find(Notification->TID_);
						if(Device!=end(ConnectingDevices_)) {
							It->second->SetDevice(Device->second);
							ConnectingDevices_.erase(Notification->TID_);
							if (!It->second->Joined() && It->second->ValidClient()) {
								It->second->Join();
								It->second->Login();
							}
						}
					} break;
					case RTTYS_Notification_type::client_registration: {
						It->second->SetClient(Notification->client_);
						if(!It->second->Joined() && It->second->ValidDevice()) {
							It->second->Join();
							It->second->Login();
						}
					} break;
					case RTTYS_Notification_type::device_connection: {
						Notification->device_->CompleteConnection();
					} break;
					case RTTYS_Notification_type::unknown: {
					} break;
					};
				} else {
					if(Notification->type_==RTTYS_Notification_type::device_connection) {
						ConnectingDevices_[Notification->TID_] = Notification->device_;
						Notification->device_->CompleteConnection();
					} else if(Notification->type_==RTTYS_Notification_type::device_registration) {
						FailedNumDevices_++;
						auto ptr = Notification->device_;
						FailedDevices.push_back(std::move(ptr));
					} else if(Notification->type_==RTTYS_Notification_type::client_registration) {
						FailedNumClients_++;
						auto ptr = Notification->client_;
						FailedClients.push_back(std::move(ptr));
					}
				}
			}
			NextNotification = ResponseQueue_.waitDequeueNotification();
		}
	}

	bool RTTYS_server::SendToClient(const std::string &Id, const u_char *Buf, std::size_t Len) {
		std::lock_guard 	Lock(LocalMutex_);

		try {
			auto It = EndPoints_.find(Id);
			if (It != EndPoints_.end() && It->second!=nullptr) {
				return It->second->SendToClient(Buf,Len);
			}
		} catch(const Poco::Exception &E) {
			Logger().log(E);
		} catch (...) {
			std::cout << "Exception in SendToClient 1" << std::endl;
		}
		return false;
	}

	bool RTTYS_server::SendToClient(const std::string &Id, const std::string &s) {
		std::lock_guard 	Lock(LocalMutex_);

		try {
			auto It = EndPoints_.find(Id);
			if (It != EndPoints_.end() && It->second!=nullptr) {
				return It->second->SendToClient(s);
			}
		} catch(const Poco::Exception &E) {
			Logger().log(E);
		} catch (...) {
			std::cout << "Exception in SendToClient 2" << std::endl;
		}
		return false;
	}

	bool RTTYS_server::SendKeyStrokes(const std::string &Id, const u_char *buffer, std::size_t len) {
		std::lock_guard 	Lock(LocalMutex_);

		auto It=EndPoints_.find(Id);
		if(It==EndPoints_.end() || It->second==nullptr) {
			return false;
		}

		try {
			auto res = It->second->KeyStrokes(buffer, len);
			return res;
		} catch(const Poco::Exception &E) {
			Logger().log(E);
		} catch (...) {
		}
		return false;
	}

	bool RTTYS_server::WindowSize(const std::string &Id, int cols, int rows) {
		std::lock_guard 	Lock(LocalMutex_);

		auto It=EndPoints_.find(Id);
		if(It==EndPoints_.end() || It->second==nullptr) {
			return false;
		}
		try {
			return It->second->WindowSize(cols,rows);
		} catch(const Poco::Exception &E) {
			Logger().log(E);
		} catch (...) {
		}
		return false;
	}

	bool RTTYS_server::CreateEndPoint(const std::string &Id, const std::string & Token, const std::string & UserName, const std::string & SerialNumber ) {
		std::lock_guard 	Lock(LocalMutex_);

		if(MaxConcurrentSessions_!=0 && EndPoints_.size()==MaxConcurrentSessions_) {
			return false;
		}

		EndPoints_[Id] = std::make_unique<RTTYS_EndPoint>(Token, SerialNumber, UserName );
		++TotalEndPoints_;
		return true;
	}

	bool RTTYS_server::ValidId(const std::string &Token) {
		std::lock_guard 	Lock(LocalMutex_);
		return EndPoints_.find(Token) != EndPoints_.end();
	}

}