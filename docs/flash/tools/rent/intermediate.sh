#!/bin/bash

dir="./pki/intermediate"
certs="$dir/certs"			# Where the issued certs are kept
crl_dir="$dir/crl"			# Where the issued crl are kept
csr_dir="$dir/csr"			# Where the issued crl are kept
database="$dir/index.txt"	# database index file.
new_certs_dir="$dir/newcerts"	# default place for new certs.
private="$dir/private"
ecparams="./pki/ecparams"

mkdir -p $certs $crl_dir $csr_dir $new_certs_dir $private $ecparams
chmod 700 $private
touch $database
echo 01 > $dir/serial
echo 01 > $dir/crlnumber

filename="`date +%y%m%d_%H%M`_intermediate"

openssl req -utf8 -new -newkey ec:"$ecparams/secp384r1.pem" -config openssl-geoscan-intermediate.cnf -keyout $private/$filename.key -sha256 -out $csr_dir/$filename.csr
openssl ca -config openssl-geoscan-rootca.cnf -extensions v3_intermediate_end -days 365 -notext -md sha256 -in $csr_dir/$filename.csr -out $certs/$filename.crt
chmod 444 $certs/$filename.crt
openssl x509 -outform der -in $certs/$filename.crt -out $certs/$filename.der
openssl x509 -noout -text -in $certs/$filename.crt
