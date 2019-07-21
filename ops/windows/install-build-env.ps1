# Download boost from sourceforge.net
wget https://netcologne.dl.sourceforge.net/project/boost/boost/1.67.0/boost_1_67_0.zip -OutFile boost_1_67_0.zip
# Unzip
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory(".\boost_1_67_0.zip", ".\")