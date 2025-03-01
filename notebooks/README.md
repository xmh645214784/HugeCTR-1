# HugeCTR Jupyter demo notebooks
This directory contains a set of Jupyter Notebook demos for HugeCTR.

## Quickstart
The quickest way to run a notebook here is with a docker container, which provides a self-contained, isolated, and reproducible environment for repetitive experiments. HugeCTR is available as buildable source code, but the easiest way to install and run HugeCTR is to use the pre-built Docker image available from the NVIDIA GPU Cloud (NGC). If you want to build the HugeCTR docker image on your own, please refer to [Use Docker Container](../docs/mainpage.md#use-docker-container).

### Pull the NGC Docker
To start the [embedding_plugin.ipynb](embedding_plugin_deprecated.ipynb) notebook, pull this docker image:
```
docker pull nvcr.io/nvstaging/merlin/merlin-tensorflow-training:0.5
```

To start the other notebooks, pull the docker image using the following command:
```
docker pull nvcr.io/nvidia/merlin/merlin-training:0.5
```

### Clone the HugeCTR Repository
Use the following command to clone the HugeCTR repository:
```
git clone https://github.com/NVIDIA/HugeCTR
```

### Start the Jupyter Notebook

1. Launch the container in interactive mode (mount the HugeCTR root directory into the container for your convenience) by running this command: 
   ```
   docker run --runtime=nvidia --rm -it -u $(id -u):$(id -g) -v $(pwd):/hugectr -w /hugectr -p 8888:8888 nvcr.io/nvidia/merlin/merlin-training:0.5
   ```  
   Launch the container in interactive mode (mount the HugeCTR root directory into the container for your convenience) by running this command to run [embedding_plugin.ipynb](embedding_plugin.ipynb) notebook : 
   ```
   docker run --runtime=nvidia --rm -it -u $(id -u):$(id -g) -v $(pwd):/hugectr -w /hugectr -p 8888:8888 nvcr.io/nvstaging/merlin/merlin-tensorflow-training:0.5
   ```

2. Activate the merlin conda environment by running the following command:  
   ```shell.
   source activate merlin
   ```

3. Start Jupyter using these commands: 
   ```
   cd /hugectr/notebooks
   jupyter-notebook --allow-root --ip 0.0.0.0 --port 8888 --NotebookApp.token=’hugectr’
   ```

4. Connect to your host machine using the 8888 port by accessing its IP address or name from your web browser: `http://[host machine]:8888`

   Use the token available from the output by running the command above to log in. For example:

   `http://[host machine]:8888/?token=aae96ae9387cd28151868fee318c3b3581a2d794f3b25c6b`


**NOTE**
- HugeCTR is built and installed with `MULTINODES=ON` within NGC Merlin docker. To use HugeCTR Python interface correctly, you should add `from mpi4py import MPI` in the scripts that `import hugectr`.
- HugeCTR is written in CUDA/C++ and wrapped to Python using Pybind11. The C++ output will not display in Notebook cells unless you run the Python script in a command line manner.


## Notebook List
The notebooks are located within the container and can be found here: `/hugectr/notebooks`.

Here's a list of notebooks that you can run:
- [ecommerce-example.ipynb](ecommerce-example.ipynb): Explains how to train and inference with the eCommerce dataset.
- [movie-lens-example.ipynb](movie-lens-example.ipynb): Explains how to train and inference with the MoveLense dataset.
- [embedding_plugin.ipynb](embedding_plugin_deprecated.ipynb): Explains how to install and use the HugeCTR embedding plugin with Tensorflow. Please be noted that this library is deprecated, and its features are moved to [sparse_operation_kit](sparse_operation_kit_demo.ipynb).
- [sparse_operation_kit_demo.ipynb](sparse_operation_kit_demo.ipynb): Demos of new TensorFlow plugins for sparse operations (embedding layers).
- [hugectr-criteo.ipynb](hugectr_criteo.ipynb): Explains the usage of HugeCTR Python interface with the Criteo dataset.
