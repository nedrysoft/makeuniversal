# makeuniversal

This application can be used to generate a universal build of Qt from the x86_64 and ARM64 build folders.

It uses rsync to copy the x86_64 folder to the destination and then finds all binaries and adds the arm64 arch to them to create a Universal binary.

## Usage

```makeuniversal <universal> <x86_64> <arm64>```

* universal is the folder which will contain the universal version of Qt,
* x86_64 is the qtbase folder in the x86_64 build folder.
* arm64 is the qtbase folder in the arm build folder.

## Example

```./makeuniversal "./arm-mac-universal" "./arm-mac-x86_64" "./arm-mac-arm64"```

## License

makeuniversal is licensed under the GPLv3 License, if you would like to use the library and this is not compatible then you should contact us to discuss options.
