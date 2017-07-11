# TOAD Ground Station Backend

##Setup (Ubuntu):
0. (Optional) create and activate virtual environment
```
virtualenv -p python3 venv
source venv/bin/activate
```

<br>

1. Install llvmlite (for numba https://github.com/numba/numba#custom-python-environments)
```
sudo apt-get install llvm
git clone https://github.com/numba/llvmlite
cd llvmlite
python setup.py install
```

<br>

2. Install python dependencies via pip3
```
pip3 install -r requirements.txt
```
<br><br>
To be used with Martlet III GCS
