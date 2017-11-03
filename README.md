Example code for Oculus Rift.

git clone --recursive git@github.com:CatCuddler/Game.git
cd Game/Kore/
git checkout master
git pull
git submodule update --recursive --init
cd ..

node Kore/make -g openGL
Additionally use --vr oculus to compule for VR.

Open Project in build Directory.