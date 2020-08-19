# SPGridVDB

Private respository to experiment with the combination of SPGrid and VDB

##### Building OpenVDB
```
cd SPGridVDB
mkdir openvdb/build
cd openvdb/build
cmake -DCMAKE_INSTALL_PREFIX =/path/to/install/dir -DOPENVDB_BUILD_PYTHON_MODULE=OFF -DOPENVDB_BUILD_BINARIES=OFF ..
make -j 12 install
```
(see https://www.openvdb.org/documentation/doxygen/build.html for more details)

##### Build and run tests
```
cd SPGridVDB
make -j all
./release/TestLaplacian
./debug/TestLaplacian
```