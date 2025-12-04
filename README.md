# COSC 522 Project #2: Following Idol Posts using TCP Sockets

Authors: **Ardeshir (Ari) Hassani and Welby Seely**
Github Repository: https://github.com/crimsonmagick/cosc-522-project-2

## Building project
First, either clone or unzip the project in the parent directory of your choice:

e.g. cloning with ssh:
```
git clone git@github.com:crimsonmagick/cosc-522-project-2.git
```

Next, change into the directory:

```
cd cosc-522-project-2
```

Create a build directory and change into it:
```
mkdir build && cd build
```

Run Cmake and Make to build the application:

```
cmake .. && make
```

Check the directory with `ls` and you should see the following files:

```
CMakeCache.txt  CMakeFiles  cmake_install.cmake  lodi_client  lodi_server  Makefile  pke_server  rsa_generate  tfa_client  tfa_server
```

There should be 5 executables in the directory for each of the 5 client/server programs: `lodi_client`, `tfa_client`,
`pke_server`, `tfa_server`, and `lodi_server`.

There is an additional `rsa_generate` for generating a toy RSA private/public key pair.


## Running each of the Lodi applications

### Use Tmux to run all 5 programs interactively

We recommend using `tmux` to run each of the applications. First, create 5 tmux sessions for each of the programs:

```
tmux new -d -s lodi_client
tmux new -d -s tfa_client
tmux new -d -s lodi_server 
tmux new -d -s pke_server
tmux new -d -s tfa_server
```

Verify that you have 5 new tmux sessions with `tmux list-sessions`, e.g.:

```
[awesomestudent@emunix.emich.edu:/usr/users/students/astudent/build_test/cosc-522-project-1/build]# tmux list-sessions
lodi_client: 1 windows (created Fri Nov  7 16:21:51 2025) [80x24]
lodi_server: 1 windows (created Fri Nov  7 16:21:51 2025) [80x24]
pke_server: 1 windows (created Fri Nov  7 16:21:51 2025) [80x24]
tfa_client: 1 windows (created Fri Nov  7 16:21:51 2025) [80x24]
tfa_server: 1 windows (created Fri Nov  7 16:21:53 2025) [80x24]
```

### Attaching to a session:

Attach to a tmux session using `tmux a -t {session_name}.

e.g. for Lodi Client:

```
tmux a -t lodi_client
```

### Running a program and switching sessions:


Let's start the lodi_server program:

```
./lodi_server
```

You should see the program running successfully:

```
Primary local IP: 127.0.0.1
Configured PKE client with address=127.0.0.1, port=9091
[DEBUG] Attempting to listen...
Listen initiated!
Primary local IP: 127.0.0.1
```

To switch to another session, press `ctrl+b s`. You should see all 5 sessions:

```
(0)  + lodi_client: 1 windows (attached)
(1)  + lodi_server: 1 windows
(2)  + pke_server: 1 windows
(3)  + tfa_client: 1 windows
(4)  + tfa_server: 1 windows
```

Use the up and down arrows to select another session, and press enter to select the session.

### Commands for running each of the 5 main programs
1. Lodi Client
    * `./lodi_client`
2. TFA Client
    * `./tfa_client`
3. Lodi Server
    * `./lodi_server`
4. PKE Server
    * `./pke_server`
5. TFA Server
    * `./tfa_server`

As per specs, only the clients can be interacted with directly. The server sessions will only have reactive output.

### Important Note
Please start up each of the servers before running the client programs. The clients will only properly connect if the
server ports are open.

### Generating a public/private key pair
Optionally use the `rsa_generate` program to generate the private/public key pair:

```
./rsa_generate
publicKey=17, privateKey=470713419953, modulus=1000268017667
timestamp=1762552703, encrypted=919744651290, decrypted=1762552703
Encryption/decryption success!
```

Currently, the generation is hardcoded, so running this program is optional. You can directly use `17` as your public key
and `470713419953` for your private key without running the generation program.

Note that the modulus is also currently hard-coded.

## Project Structure

The project is built with CMake, using C99 as the C standard. 

There are 5 targets, one for each executable. Header files and shared source files are included using
`include_directories` and `file(GLOB COMMON_SRC CONFIGURE_DEPENDS...)` respectively.

### Project Includes

All shared source files use headers (`.h` files) to establish public interfaces. These can be found under 
`{project_root}/include`. 

There are three major subdirectories in `include`:

1. `collections`
   * Implementations for the two collection data structures used in the project
       1. `int_map.h` Hash Map using `int` as the key type
       2. `list.h` Linked List implementation
2. `domain`
   * Shared interfaces for interactions between the 5 programs are provided in here:
     1. `lodi.h` for the "Lodi" domain
     2. `pke.h` for the "Public Key Services" domain
     3. `tfa.h` for the "Two Factor Authentication" domain
3. `util`
   * Shared interfaces for common general-use functionality
     1. `buffers.h` for managing buffers and byte-order
     2. `input.h` utility functions for user input
     3. `network.h` for raw UDP interactions and functionality
     4. `rsa.h` for providing core encryption functionality
     5. `server_configs.h` for providing easy access to server configuration information (addresses and ports)
4. root header files
   * There is one other header file in the root `include` dir
     1. `shared.h`, providing common constants for the application

## Project Source Implementation Files

All `.c` implementation files can be found under `{project_root}/src`. Each of the programs has its own dedicated folder,
containing one or more source files for the component.

`{project_root}/src/shared` contains implementation code for the public interfaces included in the `{project_root}/src/shared`
directory. The files and directory structure mirror the "include" directory structure described above.

### RSA Key Generation

For purposes of this project, we generate a usable private/public key pair under the "rsa_generate" target. Its source 
can be found in `{project_root}/src/rsa-utils/rsa_generate.c`. This is one additional target built by CMake.

## GCP Screenshots
Can be found in [GCP_SCREENSHOTS.md](GCP_SCREENSHOTS.md)




