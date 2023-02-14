
.PHONY: all install gen_ca_cert gen_server_cert clean

# Files generated:
#     ca_private_key.pem: CA unencrypted private key in PEM
#     ca_public_key.pem:  CA public key in PEM
#     ca.csr:             CA Certificate Signing Request
#     ca_cert.pem:        CA Certificate
#
#     server_private_key.pem: server unencrypted private key in PEM
#     server_public_key.pem:  server public key in PEM
#     server.csr:             server Certificate Signing Request
#     server_cert.pem:        server Certificate
#
# Config files:
# ca_csr.conf:         config file for creating CA CSR
# server_csr.conf:     config file for creating server CSR

INSTALL_DIR := ./ssl_cert_build

all: my_certs_test.h;

my_certs_test.h: gen_ca_cert gen_server_cert ca_bundle.pem
	./gencertbuf.pl

gen_server_cert:
	openssl genrsa -out server_private_key.pem # Generate public/private key pair
	openssl rsa -inform PEM -in server_private_key.pem -outform PEM -pubout -out server_public_key.pem # #extract public key
	openssl req -new -config server_csr.conf -key server_private_key.pem -out server.csr # Generate certificate request
	openssl x509 -days 7000 -keyform PEM -req -inform PEM -in server.csr -CA ca_cert.pem -CAcreateserial -CAkey ca_private_key.pem -outform PEM -out server_cert.pem # Generate self-signed certificate from the request


gen_ca_cert:
	openssl genrsa -out ca_private_key.pem # Generate public/private key pair
	openssl rsa -inform PEM -in ca_private_key.pem -outform PEM -pubout -out ca_public_key.pem # #extract public key
	openssl req -new -config ca_csr.conf -key ca_private_key.pem -out ca.csr # Generate certificate request
	openssl x509 -days 7000 -keyform PEM -signkey ca_private_key.pem -req -in ca.csr -outform PEM -out ca_cert.pem # Generate self-signed certificate from the request

ca_bundle.pem:
	cat server_cert.pem > $@
	cat ca_cert.pem >> $@

install:
	cp ca_bundle.pem server_cert.pem server_private_key.pem my_certs_test.h $(INSTALL_DIR)/
clean:
	rm -f *.csr *.pem my_certs_test.h *.srl
