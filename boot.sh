#! /bin/sh
cd libcetcd/&& make 
cd -
autoreconf --install --force
