#!/bin/bash
set -e

if [ "$SELFSIGNED_CERTS" = 'true' ]; then
    update-ca-certificates
fi

if [[ "$TEMPLATE_CONFIG" = 'true' ]]; then
    WEBSOCKET_HOST_ROOTCA=${WEBSOCKET_HOST_ROOTCA:-"\$OWGW_ROOT/certs/root.pem"} \
    WEBSOCKET_HOST_ISSUER=${WEBSOCKET_HOST_ISSUER:-"\$OWGW_ROOT/certs/issuer.pem"} \
    WEBSOCKET_HOST_CERT=${WEBSOCKET_HOST_CERT:-"\$OWGW_ROOT/certs/websocket-cert.pem"} \
    WEBSOCKET_HOST_KEY=${WEBSOCKET_HOST_KEY:-"\$OWGW_ROOT/certs/websocket-key.pem"} \
    WEBSOCKET_HOST_CLIENTCAS=${WEBSOCKET_HOST_CLIENTCAS:-"\$OWGW_ROOT/certs/clientcas.pem"} \
    WEBSOCKET_HOST_CAS=${WEBSOCKET_HOST_CAS:-"\$OWGW_ROOT/certs/cas"} \
    WEBSOCKET_HOST_PORT=${WEBSOCKET_HOST_PORT:-"15002"} \
    WEBSOCKET_HOST_KEY_PASSWORD=${WEBSOCKET_HOST_KEY_PASSWORD:-"mypassword"} \
    RESTAPI_HOST_ROOTCA=${RESTAPI_HOST_ROOTCA:-"\$OWGW_ROOT/certs/restapi-ca.pem"} \
    RESTAPI_HOST_PORT=${RESTAPI_HOST_PORT:-"16002"} \
    RESTAPI_HOST_CERT=${RESTAPI_HOST_CERT:-"\$OWGW_ROOT/certs/restapi-cert.pem"} \
    RESTAPI_HOST_KEY=${RESTAPI_HOST_KEY:-"\$OWGW_ROOT/certs/restapi-key.pem"} \
    RESTAPI_HOST_KEY_PASSWORD=${RESTAPI_HOST_KEY_PASSWORD:-"mypassword"} \
    INTERNAL_RESTAPI_HOST_ROOTCA=${INTERNAL_RESTAPI_HOST_ROOTCA:-"\$OWGW_ROOT/certs/restapi-ca.pem"} \
    INTERNAL_RESTAPI_HOST_PORT=${INTERNAL_RESTAPI_HOST_PORT:-"17002"} \
    INTERNAL_RESTAPI_HOST_CERT=${INTERNAL_RESTAPI_HOST_CERT:-"\$OWGW_ROOT/certs/restapi-cert.pem"} \
    INTERNAL_RESTAPI_HOST_KEY=${INTERNAL_RESTAPI_HOST_KEY:-"\$OWGW_ROOT/certs/restapi-key.pem"} \
    INTERNAL_RESTAPI_HOST_KEY_PASSWORD=${INTERNAL_RESTAPI_HOST_KEY_PASSWORD:-"mypassword"} \
    FILEUPLOADER_HOST_ROOTCA=${FILEUPLOADER_HOST_ROOTCA:-"\$OWGW_ROOT/certs/restapi-ca.pem"} \
    FILEUPLOADER_HOST_NAME=${FILEUPLOADER_HOST_NAME:-"localhost"} \
    FILEUPLOADER_HOST_PORT=${FILEUPLOADER_HOST_PORT:-"16003"} \
    FILEUPLOADER_HOST_CERT=${FILEUPLOADER_HOST_CERT:-"\$OWGW_ROOT/certs/restapi-cert.pem"} \
    FILEUPLOADER_HOST_KEY=${FILEUPLOADER_HOST_KEY:-"\$OWGW_ROOT/certs/restapi-key.pem"} \
    FILEUPLOADER_HOST_KEY_PASSWORD=${FILEUPLOADER_HOST_KEY_PASSWORD:-"mypassword"} \
    FILEUPLOADER_PATH=${FILEUPLOADER_PATH:-"\$OWGW_ROOT/uploads"} \
    FILEUPLOADER_URI=${FILEUPLOADER_URI:-"https://localhost:16003"} \
    SERVICE_KEY=${SERVICE_KEY:-"\$OWGW_ROOT/certs/restapi-key.pem"} \
    SERVICE_KEY_PASSWORD=${SERVICE_KEY_PASSWORD:-"mypassword"} \
    SYSTEM_DATA=${SYSTEM_DATA:-"\$OWGW_ROOT/data"} \
    SYSTEM_URI_PRIVATE=${SYSTEM_URI_PRIVATE:-"https://localhost:17002"} \
    SYSTEM_URI_PUBLIC=${SYSTEM_URI_PUBLIC:-"https://localhost:16002"} \
    SYSTEM_URI_UI=${SYSTEM_URI_UI:-"http://localhost"} \
    SECURITY_RESTAPI_DISABLE=${SECURITY_RESTAPI_DISABLE:-"false"} \
    SIMULATORID=${SIMULATORID:-""} \
    IPTOCOUNTRY_PROVIDER=${IPTOCOUNTRY_PROVIDER:-"ipinfo"} \
    IPTOCOUNTRY_IPINFO_TOKEN=${IPTOCOUNTRY_IPINFO_TOKEN:-""} \
    IPTOCOUNTRY_IPDATA_APIKEY=${IPTOCOUNTRY_IPDATA_APIKEY:-""} \
    AUTOPROVISIONING_PROCESS=${AUTOPROVISIONING_PROCESS:-"prov,default"} \
    RTTY_INTERNAL=${RTTY_INTERNAL:-"true"} \
    RTTY_ENABLED=${RTTY_ENABLED:-"true"} \
    RTTY_SERVER=${RTTY_SERVER:-"localhost"} \
    RTTY_PORT=${RTTY_PORT:-"5912"} \
    RTTY_TOKEN=${RTTY_TOKEN:-""} \
    RTTY_TIMEOUT=${RTTY_TIMEOUT:-"60"} \
    RTTY_VIEWPORT=${RTTY_VIEWPORT:-"5913"} \
    RTTY_ASSETS=${RTTY_ASSETS:-"\$OWGW_ROOT/rtty_ui"} \
    RADIUS_PROXY_ENABLE=${RADIUS_PROXY_ENABLE:-"false"} \
    RADIUS_PROXY_ACCOUNTING_PORT=${RADIUS_PROXY_ACCOUNTING_PORT:-"1813"} \
    RADIUS_PROXY_AUTHENTICATION_PORT=${RADIUS_PROXY_AUTHENTICATION_PORT:-"1812"} \
    RADIUS_PROXY_COA_PORT=${RADIUS_PROXY_COA_PORT:-"3799"} \
    KAFKA_ENABLE=${KAFKA_ENABLE:-"true"} \
    KAFKA_BROKERLIST=${KAFKA_BROKERLIST:-"localhost:9092"} \
    KAFKA_SSL_CA_LOCATION=${KAFKA_SSL_CA_LOCATION:-""} \
    KAFKA_SSL_CERTIFICATE_LOCATION=${KAFKA_SSL_CERTIFICATE_LOCATION:-""} \
    KAFKA_SSL_KEY_LOCATION=${KAFKA_SSL_KEY_LOCATION:-""} \
    KAFKA_SSL_KEY_PASSWORD=${KAFKA_SSL_KEY_PASSWORD:-""} \
    STORAGE_TYPE=${STORAGE_TYPE:-"sqlite"} \
    STORAGE_TYPE_POSTGRESQL_HOST=${STORAGE_TYPE_POSTGRESQL_HOST:-"localhost"} \
    STORAGE_TYPE_POSTGRESQL_USERNAME=${STORAGE_TYPE_POSTGRESQL_USERNAME:-"owgw"} \
    STORAGE_TYPE_POSTGRESQL_PASSWORD=${STORAGE_TYPE_POSTGRESQL_PASSWORD:-"owgw"} \
    STORAGE_TYPE_POSTGRESQL_DATABASE=${STORAGE_TYPE_POSTGRESQL_DATABASE:-"owgw"} \
    STORAGE_TYPE_POSTGRESQL_PORT=${STORAGE_TYPE_POSTGRESQL_PORT:-"5432"} \
    STORAGE_TYPE_MYSQL_HOST=${STORAGE_TYPE_MYSQL_HOST:-"localhost"} \
    STORAGE_TYPE_MYSQL_USERNAME=${STORAGE_TYPE_MYSQL_USERNAME:-"owgw"} \
    STORAGE_TYPE_MYSQL_PASSWORD=${STORAGE_TYPE_MYSQL_PASSWORD:-"owgw"} \
    STORAGE_TYPE_MYSQL_DATABASE=${STORAGE_TYPE_MYSQL_DATABASE:-"owgw"} \
    STORAGE_TYPE_MYSQL_PORT=${STORAGE_TYPE_MYSQL_PORT:-"3306"} \
    CERTIFICATES_ALLOWMISMATCH=${CERTIFICATES_ALLOWMISMATCH:-"true"} \
    envsubst < /owgw.properties.tmpl > $OWGW_CONFIG/owgw.properties
fi

# Check if rtty_ui directory exists
export RTTY_ASSETS=$(grep 'rtty.assets' $OWGW_CONFIG/owgw.properties | awk -F '=' '{print $2}' | xargs | envsubst)

if [ -z "$RTTY_ASSETS" ]; then
    export RTTY_ASSETS="$OWGW_ROOT/rtty_ui"
fi

if [[ ! -d "$(dirname $RTTY_ASSETS)" ]]; then
  mkdir -p $(dirname $RTTY_ASSETS)
fi

if [[ ! -d "$RTTY_ASSETS" ]]; then
    cp -r /dist/rtty_ui $RTTY_ASSETS
fi

if [ "$1" = '/openwifi/owgw' -a "$(id -u)" = '0' ]; then
    if [ "$RUN_CHOWN" = 'true' ]; then
        chown -R "$OWGW_USER": "$OWGW_ROOT" "$OWGW_CONFIG"
    fi
    exec gosu "$OWGW_USER" "$@"
fi

exec "$@"
