#!/bin/bash

dir="./pki"
certs="$dir/certs"			# Where the issued certs are kept
crl_dir="$dir/crl"			# Where the issued crl are kept
database="$dir/index.txt"	# database index file.
new_certs_dir="$dir/newcerts"	# default place for new certs.
private="$dir/private"
ecparams="$dir/ecparams"

if [ -d "$dir" ]; then
touch $database
echo 01 > $dir/serial
else
mkdir -p $certs $crl_dir $new_certs_dir $private $ecparams
chmod 700 $private
touch $database
echo 01 > $dir/serial
openssl ecparam -name secp384r1 -out $ecparams/secp384r1.pem
openssl req -utf8 -new -newkey ec:"$ecparams/secp384r1.pem" -config openssl-geoscan-rootca.cnf -keyout $private/ca.key -x509 -days 7300 -sha256 -extensions v3_ca -out $dir/ca.crt
chmod 444 $dir/ca.crt
openssl x509 -outform der -in $dir/ca.crt -out $dir/ca.crt.der
openssl x509 -noout -text -in $dir/ca.crt
fi