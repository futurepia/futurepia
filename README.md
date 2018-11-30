# Building Futurepia
For Ubuntu 16.04 users, after installing the right packages with `apt` Futurepia
will build out of the box without further effort:

```
sudo apt-get install -y \  
        autoconf \  
        automake \  
        cmake \  
        g++ \  
        git \  
        libssl-dev \  
        libtool \  
        make \  
        pkg-config \  
        python3 \  
        python3-jinja2
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
        doxygen \  
        libncurses5-dev \  
        libreadline-dev \  
        perl  
```

$>git clone https://github.com/futurepia/futurepia

$>git submodule update --init --recursive

$>mkdir build

$>cd build

$>cmake -DCMAKE_BUILD_TYPE=Release ..

$>make -j$(nproc) futurepiad

$>make -j$(nproc) cli_wallet




