# Cheats Collection

This is my personal cheats collection for Apex Legends, Starcraft 2, Civilization 6 and Speedrunners.

## Features

### Apex Legends

See the subrepository

### Starcraft 2

Some data decryption is implemented but development is stopped for now.

### Civilization 6

Set gold amount to spend as much as you want.

### Speedrunners

Unlimited boosting or use a boost multiplier for more stealth.

## Compile

```
1. Checkout the code
git clone https://github.com/orkido/cheats.git
cd cheats
# git submodule update --init is not supported ([see below](#git-submodule))
git clone https://github.com/DarthTon/Blackbone.git

2. Install Qt
Install Qt and set the correct paths for 32 bit AND 64 bit version in CMakeLists.txt
E.g. for default installation path:
set(CMAKE_PREFIX_PATH "C:/Qt/5.14.2/msvc2017_64") default installation

3. Open as CMake project in Visual Studio and press compile, that's it!
```

Configuration options in CMakeLists.txt can be used to enable/disable functions:
```
set(CoD_Overlay FALSE)
set(Starcraft2 FALSE)
set(Civilization TRUE)
set(Apex FALSE)
```

## git submodule
The Apex and Blackbone submodules are not available for now because the Apex Legends part requires some code cleanup. When that is done it will be published.
The Blackbone repository is mostly the same as the official with an additional Anti-Cheat bypass method that stays private for now.

## Credits

- The great memory hacking framework [Blackbone](https://github.com/DarthTon/Blackbone)
- [apexbot](https://github.com/CasualX/apexbot) a perfectly extensible Apex Legends hack with many ready to use features
- [Qt](https://www.qt.io/) GUI framework

## Contribution

Found a bug or a new feature? Please open an issue ticket or directly create a pull request.
