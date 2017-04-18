FROM it4i/loombuilder:latest

RUN apt-get update -y && apt-get install -y git

RUN git clone https://code.it4i.cz/ADAS/loom.git && \
    cd loom && \
    mkdir _build && \
    cd _build && \
    cmake .. && \
    make -j2 && \
    cd ../python && \
    sh generate.sh && \
    python3 setup.py install

ENV PYTHONPATH "$PYTHONPATH:/loom/python/"
