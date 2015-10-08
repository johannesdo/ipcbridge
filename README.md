# ipcbridge

## Installation


### without pip
```
cd ipcbridge

python setup.py build
sudo python setup.py install
```

### with pip

```
cd ipcbridge

rm dist/ MANIFEST -rf; python setup.py sdist; pip install ipcbridge --no-cache-dir --no-index -f ./dist --upgrade
```

on code changes, change the (minor) version number as well (in setup.py), or else pip may not update the installation
