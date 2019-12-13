#!/bin/bash

set -e

export LC_ALL="en_US.UTF-8"

binary_url="https://github.com/EPMCoinOfficial/EPM/releases/download/8f4ed61d4/epmcoin-v0.14.0.4-8f4ed61d4-lin64.tgz"
file_name="epmcoin-v0.14.0.4-8f4ed61d4-lin64"
extension=".tgz"

echo ""
echo "#################################################"
echo "#   Welcome to the EPMCoin Masternode Setup   #"
echo "#################################################"
echo ""

ipaddr="$(dig +short myip.opendns.com @resolver1.opendns.com)"
while [[ $ipaddr = '' ]] || [[ $ipaddr = ' ' ]]; do
	read -p 'Unable to find an external IP, please provide one: ' ipaddr
	sleep 2
done

read -p 'Please provide masternodeblsprivkey: ' mnkey
while [[ $mnkey = '' ]] || [[ $mnkey = ' ' ]]; do
	read -p 'You did not provide a masternodeblsprivkey, please provide one: ' mnkey
	sleep 2
done

echo ""
echo "###############################"
echo "#  Installing Dependencies    #"
echo "###############################"
echo ""
echo "Running this script on Ubuntu 18.04 LTS or newer is highly recommended."

sudo apt-get -y update
sudo apt-get -y install git python virtualenv ufw pwgen

echo ""
echo "###############################"
echo "#   Setting up the Firewall   #"
echo "###############################"
sudo ufw status
sudo ufw disable
sudo ufw allow ssh/tcp
sudo ufw limit ssh/tcp
sudo ufw allow 1505/tcp
sudo ufw logging on
sudo ufw --force enable
sudo ufw status
sudo iptables -A INPUT -p tcp --dport 1505 -j ACCEPT

echo ""
echo "###########################"
echo "#   Setting up swapfile   #"
echo "###########################"
sudo swapoff -a
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo "/swapfile swap swap defaults 0 0" >> /etc/fstab

echo ""
echo "###############################"
echo "#      Get/Setup binaries     #"
echo "###############################"
echo ""
wget $binary_url
if test -e "$file_name$extension"; then
echo "Unpacking EPMCoin distribution"
	tar -xzvf $file_name$extension
	rm -r $file_name$extension
	mv -v $file_name EPMCoin
	cd EPMCoin
	chmod +x epmcoind
	chmod +x epmcoin-cli
	echo "Binaries were saved to: /root/EPMCoin"
else
	echo "There was a problem downloading the binaries, please try running the script again."
	exit -1
fi

echo ""
echo "###############################"
echo "#     Configure the wallet    #"
echo "###############################"
echo ""
echo "A .EPMCoin folder will be created, if folder already exists, it will be replaced"
if [ -d ~/.EPMCoin ]; then
	if [ -e ~/.EPMCoin/epmcoin.conf ]; then
		read -p "The file epmcoin.conf already exists and will be replaced. do you agree [y/n]:" cont
		if [ $cont = 'y' ] || [ $cont = 'yes' ] || [ $cont = 'Y' ] || [ $cont = 'Yes' ]; then
			sudo rm ~/.EPMCoin/epmcoin.conf
			touch ~/.EPMCoin/epmcoin.conf
			cd ~/.EPMCoin
		fi
	fi
else
	echo "Creating .EPMCoin dir"
	mkdir -p ~/.EPMCoin
	cd ~/.EPMCoin
	touch epmcoin.conf
fi

echo "Configuring the epmcoin.conf"
echo "#----" > epmcoin.conf
echo "rpcuser=$(pwgen -s 16 1)" >> epmcoin.conf
echo "rpcpassword=$(pwgen -s 64 1)" >> epmcoin.conf
echo "rpcallowip=127.0.0.1" >> epmcoin.conf
echo "rpcport=7111" >> epmcoin.conf
echo "#----" >> epmcoin.conf
echo "listen=1" >> epmcoin.conf
echo "server=1" >> epmcoin.conf
echo "daemon=1" >> epmcoin.conf
echo "maxconnections=64" >> epmcoin.conf
echo "#----" >> epmcoin.conf
echo "masternode=1" >> epmcoin.conf
echo "masternodeblsprivkey=$mnkey" >> epmcoin.conf
echo "externalip=$ipaddr" >> epmcoin.conf
echo "#----" >> epmcoin.conf



echo ""
echo "#######################################"
echo "#      Creating systemctl service     #"
echo "#######################################"
echo ""

cat <<EOF > /etc/systemd/system/epmg.service
[Unit]
Description=EPM Global daemon
After=network.target
[Service]
User=root
Group=root
Type=forking
PIDFile=/root/.EPMCoin/epmcoin.pid
ExecStart=/root/EPMCoin/epmcoind -daemon -pid=/root/.EPMCoin/epmcoin.pid \
          -conf=/root/.EPMCoin/epmcoin.conf -datadir=/root/.EPMCoin/
ExecStop=-/root/EPMCoin/epmcoin-cli -conf=/root/.EPMCoin/epmcoin.conf \
          -datadir=/root/.EPMCoin/ stop
Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5
[Install]
WantedBy=multi-user.target
EOF

#enable the service
systemctl enable epmg.service
echo "epmg.service enabled"

#start the service
systemctl start epmg.service
echo "epm.g service started"


echo ""
echo "###############################"
echo "#      Running the wallet     #"
echo "###############################"
echo ""
cd ~/EPMCoin
sleep 60

is_epm_running=`ps ax | grep -v grep | grep epmcoind | wc -l`
if [ $is_epm_running -eq 0 ]; then
	echo "The daemon is not running or there is an issue, please restart the daemon!"
	exit
fi

cd ~/EPMCoin
./epmcoin-cli getinfo

echo ""
echo "Your masternode server is ready!"
