## Example code for Oculus Rift.

git clone --recursive git@github.com:CatCuddler/Game.git

cd Game/Kore/

git checkout master

git pull

git submodule update --recursive --init

cd ..

node Kore/make -g openGL

or

node Kore/make -g direct3d11

Additionally use --vr oculus to compule for Oculus Rift.

Open Project in build Directory.