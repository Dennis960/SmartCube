## Manual ARM Embedded Toolchain Installation

> **Note:** ARM deprecated the PPA distribution method. All new packages are no longer released on Launchpad, requiring manual installation of the latest `arm-none-eabi-gdb`.

### 1. Remove existing package

```bash
sudo apt remove gcc-arm-none-eabi
```

### 2. Download and extract toolchain

Download the latest Linux x86_64 Tarball from [ARM's website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm), verify its MD5, then extract:

```bash
sudo tar xjf gcc-arm-none-eabi-YOUR-VERSION.bz2 -C /usr/share/
```

### 3. Create system-wide symlinks

```bash
sudo ln -s /usr/share/gcc-arm-none-eabi-YOUR-VERSION/bin/arm-none-eabi-gcc /usr/bin/arm-none-eabi-gcc
sudo ln -s /usr/share/gcc-arm-none-eabi-YOUR-VERSION/bin/arm-none-eabi-g++ /usr/bin/arm-none-eabi-g++
sudo ln -s /usr/share/gcc-arm-none-eabi-YOUR-VERSION/bin/arm-none-eabi-gdb /usr/bin/arm-none-eabi-gdb
sudo ln -s /usr/share/gcc-arm-none-eabi-YOUR-VERSION/bin/arm-none-eabi-size /usr/bin/arm-none-eabi-size
sudo ln -s /usr/share/gcc-arm-none-eabi-YOUR-VERSION/bin/arm-none-eabi-objcopy /usr/bin/arm-none-eabi-objcopy
```

### 4. Install dependencies

```bash
sudo apt install libncurses-dev
sudo ln -s /usr/lib/x86_64-linux-gnu/libncurses.so.6 /usr/lib/x86_64-linux-gnu/libncurses.so.5
sudo ln -s /usr/lib/x86_64-linux-gnu/libtinfo.so.6 /usr/lib/x86_64-linux-gnu/libtinfo.so.5
```

### 5. Verify installation

```bash
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
arm-none-eabi-gdb --version
arm-none-eabi-size --version
```
