//
// Created by stephane bourque on 2022-08-15.
//

#pragma once

#include <iostream>
#include <fstream>

#include "RESTObjects/RESTAPI_GWobjects.h"

#include "framework/MicroService.h"

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SecureStreamSocket.h"
#include "Poco/Net/Context.h"
#include "Poco/Crypto/X509Certificate.h"
#include "Poco/Net/NetException.h"
#include "Poco/TemporaryFile.h"

#include "RADIUS_helpers.h"
#include "DeviceRegistry.h"

namespace OpenWifi {

	class RADSECserver : public Poco::Runnable {
	  public:
		RADSECserver(Poco::Net::SocketReactor & R, GWObjects::RadiusProxyServerEntry E, Poco::Logger &L) :
 			Reactor_(R),
			Server_(std::move(E)),
	  		Logger_(L) {

			MakeSecurityFiles();
			ReconnectorThr_.start(*this);
		}

		inline void run() final {
			std::cout << "About to start RADSEC connection..." << std::endl;
			while(TryAgain_) {
				if(!Connected_) {
					std::cout << "RADSEC: Trying to connect" << std::endl;
					Connect();
				}
				Poco::Thread::trySleep(1000);

			}
		}

		inline bool SendData(const unsigned char *buffer, int length) {
			if(Connected_) {
				std::cout << "RADSEC: Sending " << length << " bytes" << std::endl;
				return (Socket_->sendBytes(buffer,length) == length);
			}
			return false;
		}

		inline void onData([[maybe_unused]] const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf) {
			Poco::Buffer<char> IncomingRadiusPacket(0);
			try {
				auto NumberOfReceivedBytes = Socket_->receiveBytes(IncomingRadiusPacket);
				auto *RP = (const OpenWifi::RADIUS::RawRadiusPacket *)(IncomingRadiusPacket.begin());
				Logger_.information(fmt::format("RADSEC: received {} bytes.", NumberOfReceivedBytes));
				std::cout << "RADSEC: Received " << NumberOfReceivedBytes << " bytes" << std::endl;
				RADIUS::RadiusPacket	P(IncomingRadiusPacket);
				if(RADIUS::IsAuthentication(RP->code) ) {
					auto SerialNumber = P.ExtractSerialNumberFromProxyState();
					DeviceRegistry()->SendRadiusAuthenticationData(SerialNumber, (const unsigned char *)IncomingRadiusPacket.begin(), IncomingRadiusPacket.size());
				} else if(RADIUS::IsAccounting(RP->code)) {
					auto SerialNumber = P.ExtractSerialNumberFromProxyState();
					DeviceRegistry()->SendRadiusAccountingData(SerialNumber, (const unsigned char *)IncomingRadiusPacket.begin(), IncomingRadiusPacket.size());
				} else if(RADIUS::IsAuthority(RP->code)) {

				}

			} catch (const Poco::Exception &E) {
				Logger_.log(E);
			}
		}

		inline void onError([[maybe_unused]] const Poco::AutoPtr<Poco::Net::ErrorNotification>& pNf) {
			std::cout << "onError" << std::endl;
			Disconnect();
		}

		inline void onShutdown([[maybe_unused]] const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf) {
			std::cout << "onShutdown" << std::endl;
			Disconnect();
		}

		inline bool Connect() {
			if(TryAgain_) {

				std::cout << __LINE__ << std::endl;
				Poco::Net::Context::Ptr SecureContext = Poco::AutoPtr<Poco::Net::Context>(
					new Poco::Net::Context(Poco::Net::Context::CLIENT_USE,
										   KeyFile_.path(),
										   CertFile_.path(),""));

				std::cout << __LINE__ << std::endl;
				for(const auto &ca:CacertFiles_) {
					std::cout << __LINE__ << std::endl;
					Poco::Crypto::X509Certificate	cert(ca.path());
					std::cout << __LINE__ << std::endl;
					SecureContext->addCertificateAuthority(cert);
					std::cout << __LINE__ << std::endl;
				}

				std::cout << __LINE__ << std::endl;
				auto tmp_Socket_ = std::make_unique<Poco::Net::SecureStreamSocket>(SecureContext);
				std::cout << __LINE__ << std::endl;

				Poco::Net::SocketAddress Destination(Server_.ip, Server_.port);
				std::cout << __LINE__ << std::endl;

				try {
					std::cout << "RADSEC: trying to connect to " << Server_.ip << ":" << Server_.port << std::endl;
					tmp_Socket_->connect(Destination, Poco::Timespan(10, 0));
					std::cout << "RADSEC: Connected" << std::endl;

					Reactor_.addEventHandler(
						*tmp_Socket_,
						Poco::NObserver<RADSECserver, Poco::Net::ReadableNotification>(
							*this, &RADSECserver::onData));
					Reactor_.addEventHandler(
						*tmp_Socket_, Poco::NObserver<RADSECserver, Poco::Net::ErrorNotification>(
										  *this, &RADSECserver::onError));
					Reactor_.addEventHandler(
						*tmp_Socket_,
						Poco::NObserver<RADSECserver, Poco::Net::ShutdownNotification>(
							*this, &RADSECserver::onShutdown));
					Socket_ = std::move(tmp_Socket_);
					Connected_ = true;
					std::cout << "Connected to RADSEC" << std::endl;
					return true;
				} catch (const Poco::Net::NetException &E) {
					std::cout << "NetException: " << E.name() << " " << E.what() << std::endl;
					Logger_.log(E);
				} catch (const Poco::Exception &E) {
					std::cout << "Exception: " << E.name() << " " << E.what() << std::endl;
					Logger_.log(E);
				}
			}
			return false;
		}

		inline void Disconnect() {
			Reactor_.removeEventHandler(*Socket_,Poco::NObserver<RADSECserver, Poco::Net::ReadableNotification>(
												   *this, &RADSECserver::onData));
			Reactor_.removeEventHandler(*Socket_,Poco::NObserver<RADSECserver, Poco::Net::ErrorNotification>(
												   *this, &RADSECserver::onError));
			Reactor_.removeEventHandler(*Socket_,Poco::NObserver<RADSECserver, Poco::Net::ShutdownNotification>(
												   *this, &RADSECserver::onShutdown));
			Socket_->shutdown();
		}

		inline void Stop() {
			TryAgain_ = false;
			Disconnect();
			ReconnectorThr_.wakeUp();
			ReconnectorThr_.join();
		}

		static void DecodeFile(const std::string &filename, const std::string &s) {
			std::ofstream cert_file(filename,std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
			std::stringstream is(s);
			Poco::Base64Decoder	ds(is);
			Poco::StreamCopier::copyStream(ds,cert_file);
			cert_file.close();
		}

		inline void MakeSecurityFiles() {
			DecodeFile(CertFile_.path(), Server_.radsec_cert);
			DecodeFile(KeyFile_.path(), Server_.radsec_key);

			for(auto &cert:Server_.radsec_cacerts) {
				Poco::TemporaryFile		NewFile(MicroService::instance().DataDir());
				DecodeFile(NewFile.path(), cert);
				CacertFiles_.push_back(std::move(NewFile));
			}
		}

	  private:
		Poco::Net::SocketReactor							&Reactor_;
		GWObjects::RadiusProxyServerEntry					Server_;
		Poco::Logger										&Logger_;
		std::atomic_bool 									Connected_=false;
		std::atomic_bool 									TryAgain_=true;
		std::unique_ptr<Poco::Net::SecureStreamSocket>		Socket_;
		Poco::TemporaryFile									CertFile_{MicroService::instance().DataDir()} ,
															KeyFile_{MicroService::instance().DataDir()};
		std::vector<Poco::TemporaryFile>					CacertFiles_;
		Poco::Thread										ReconnectorThr_;
	};
}