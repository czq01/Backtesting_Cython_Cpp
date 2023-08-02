set dir=%~dp0
cd %dir%
python setup.py build_ext --inplace > log/build_log.txt

del *.cpp *.html
mkdir log
move /Y *.pyd package/