# SuccinctEdgeWithStreaming

To run the test, [Succinct Data Structure Library(sdsl)](https://github.com/simongog/sdsl-lite) must be installed in `/usr/local/`.
To run, a MQTT broker must be running on the machine and the LibMosquitto library must be installed, found at https://mosquitto.org/

## Compile

    cmake ./

CMake version 3.14 is required, if there is a version problem, users may change the version number in CMakeLists.txt directly. This configuration has already been tested with Raspbian and MacOS. <br>

    make
    
An executable file named `SuccinctEdge_cata` will be generated.

## Run the test

The execution command is in the form of:

    ./SuccinctEdge_cata data_path query_path persist_path tbox_path reasoning show_result active_plan mini_batch server

##Generating Sensors

A jar called sensor_generator.jar can be used to generate sensors and values to test SuccinctEdgeWithStreaming. The jar is used with the following command line

	java -jar sensor_generator.jar [Number of sensors] [Mosquitto host (localhost or the host's IP)] [Frequency of mesures in ms]

