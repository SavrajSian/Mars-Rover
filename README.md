# Mars Rover Project
The aim was to create a rover that would autonomously navigate and map out distinct features in an arena, avoiding obstacles along the way.

### My contribution
I worked on the server aspect of the command submodule, which was created using Node.js and relevant libraries such as Express. The role of the server is to retrieve data from the MySQL database used by the rover, store relevant information in a DynamoDB database and send that data back to the client. It was also used to send remote control commands to the rover. Details of the design and exact functionality can be found in the [report](Mars-Rover-Report.md) included in this repository.

I also aided in the integration of the all the submodules in the ESP32 microcontroller, using C++.

