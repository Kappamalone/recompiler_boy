mkdir -p build && cd build
cmake -Wno-dev -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE="DEBUG"  ..
ln -rsf compile_commands.json ..
ninja
cp ./TEMPLATE ..
cd ..
./TEMPLATE $1
rm ./TEMPLATE
