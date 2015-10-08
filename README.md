# ipcbridge

## installation


### without pip
```
cd ipcbridge

python setup.py build
sudo python setup.py install
```

### with pip

```
cd ipcbridge

rm dist/ MANIFEST -rf
python setup.py sdist
pip install ipcbridge --no-cache-dir --no-index -f ./dist --upgrade
```

on code changes, change the (minor) version number as well (in setup.py), or else pip may not update the installation


# usage

```
import ipcbridge

ipcbridge.send("stuff")
msg = ipcbridge.read()
```

On import the module connects to **/tmp/ipcbridge.unix**.
The import hangs if no unix socket server accepts the connection.

An example implementation of an unix socket server is provided with **uss.c**.
Compile and execute with
```
gcc -o uss uss.c -lpthread
./uss
```

An example script is provided with **test_client.py**.
