# Building Futurepia
## For Ubuntu 16.04
```
sudo apt-get install -y \
    autoconf \
    automake \
    cmake \
    g++ \
    git \
    libbz2-dev \
    libsnappy-dev \
    libssl-dev \
    libtool \
    make \
    pkg-config \
    python3 \
    python3-jinja2 \
    doxygen
```

```
sudo apt-get install -y \
    libboost-chrono-dev \
    libboost-context-dev \
    libboost-coroutine-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-locale-dev \
    libboost-program-options-dev \
    libboost-serialization-dev \
    libboost-signals-dev \
    libboost-system-dev \
    libboost-test-dev \
    libboost-thread-dev
```

```
sudo apt-get install -y \
    libncurses5-dev \
    libreadline-dev \
    perl
```

```
git clone https://github.com/futurepia/futurepia
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) futurepiad
make -j$(nproc) cli_wallet
```

## For Windows
 Cross compile from Linux to Windows using cygwin

1. Install cygwin 
- Install packages i.e devel, perl, util, debug 

2. After running cygwin, install apt-cyg
```
wget raw.github.com/transcode-open/apt-cyg/master/apt-cyg
```

3. Install other programs (optional)
```
apt-cyg install bzip2 gzip m4 make unzip zip
```

4. Installl the python jinja2 module

5. Adding a cygwin installation path to window environment variable 
- Add CYWIN_HOME from System Properties -> Environment variables to C:\cygwin64  
- Add %CYGWIN_HOME%\bin to PATH

6. Boost Library 1.58.0 version build

7. OpenSSL build

8. Futurepia build

 8.1 Setup build environment (inside build folder)
 ```
 ../programs/build_helpers/configure_build.py --openssl-dir=/usr/x86_64-w64-mingw32 --boost-dir=/usr/local --sys-root=/usr/bin -f -r --win
```

testnet
```
 ../programs/build_helpers/configure_build.py --openssl-dir=/usr/x86_64-w64-mingw32 --boost-dir=/usr/local --sys-root=/usr/bin -f -r --win --test
 ```

 8.2 build
 ```
 make -j6 futurepiad
 make -j6 cli_wallet
 ```
 
# H/W requirements.
```
HDD free space 54G or more
CPU dual core or more
RAM 4 GB or more
OS
Linux: Ubuntu 16.04 or later
Window: Windows 10
```

# How to run the Node Program
## Linux
1. Move to the directory where the futurepiad file is located, execute the node program with the following command
   > ./futurepiad
## Windows
1. Open Explorer and move to folder with futurepiad.exe
2. Right-click on the folder while holding down the shift key on the keyboard
3. Select "Open powerShell window here" in the pop-up menu
4. Check if the folder name is same as futurepiad.exe and if it is the same, execute the node program with the following command
   >.\futurepiad.exe

# Setting up Node program 
Modify the node_data_dir/config.ini file created after the first node execution.
After modifying config.ini, you must restart node program.

## Frequently used settings in config.ini Description
```
. p2p-endpoint: ip, port settings for this node
. seed-node: ip, port setting of node to receive block data (multiple setting possible)
. rpc-endpoint: settings for ip or port to be used when the wallet or API communicates with this node through rpc.
. public-api: settings for api to be used on this node
. enable-plugin: settings for plugin to be used on this node
. bobserver: Setting up the BP or BO on this node (multiple setting possible)
. private-key: Block creation key for BP and BO that were setup in bobserver (in the same order as the order of bobserver during multiple setting)
```

# How to run wallet program
## Linux
1. For ubuntu 18.0.4
Execute the following command to connect the readline package version.
```
sudo ln -s /lib/x86_64-linux-gnu/libreadline.so.7/lib/x86_64-linux-gnu/libreadline.so.6
```
2. Move to the directory where the cli_wallet file is located and run the wallet program executing the following command
```
> ./cli_wallet
```
## Windows
1. Run the node program 1 through 3 in order.
2. Check if folder name is same as cli_wallet.exe.
```
> ./cli_wallet.exe
```

# Using wallet
You must set the password to be used for this wallet when running the wallet program for first time or when the command prompt is marked as new.
** If you lose this password, you will not be able to use the information stored in the existing wallet, so do not lose it. **

```
      set_password [wallet password]
      new >>> set_password
```

## Unlock wallet
If command prompt is locked, other commands can not be used.
     
```
      unlock [wallet password]  
      locked >>> unlock
```

## Key registration
When executing commands such as create / update etc, the active private key of BP / BO is needed, so the key is included in the wallet.

```
      import_key [private key]
      unlocked >>> import_key 5Jt2PbJHF9zswdhnypctCDoJ1uufApC2dX6H8JiQsKkjjCiwSAv
```

## Create an account
```
      create_account [BP / BO account id] [account id to create] [json meta data (empty string possible)] true  
      unlocked >>> create_account oneofall test001 "" true
```  
## Account information inquiry
```
      get_account [account id]
      unlocked >>> get_account test001
```  

## coin transfer
```
      transfer [sender account id] [recipient account id] [amount sent] [note] true   
      unlocked >>> transfer test001 test002 "100.00000000 FPC" "" true
```

