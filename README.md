This is a fork of the OpenFace project.
I modified the code to add the streaming of face and head data to a Python script through ZeroMQ on Windows.
I just needed it for a research project, and I relaized that it was not available anywhere, so I decided to implement it (though in a quick and dirt way).
Hopefully it can be of help for other people.

Instructions:
- clone the repository
- open OpenFace.sln in Visual Studio 2019
- choose the configuration x64/Release
- build the solution
- go to x64/Release and launch OpenFaceOffline.exe
- do not change any parameter, just start the webcam from the first menu

The data is streamed through a ZeroMQ PUSH (fire-and-forget) socket on 127.0.0.1:5000.

An example consumer script is provided (OpenFace_worker.py).
