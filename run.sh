mkdir -p build && cd build
cmake -Wno-dev -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=$1  ..
ln -rsf compile_commands.json ..
ninja
cp ./TEMPLATE ..
cd ..
./TEMPLATE $2 $3
rm ./TEMPLATE
