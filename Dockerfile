FROM it4i/loombuilder:latest

ADD . /loom

RUN cd loom && \
    mkdir _build && \
    cd _build && \
    cmake .. && \
    make -j2 && \
    cd ../python && \
    sh generate.sh && \
    python3 setup.py install

ENV PYTHONPATH "$PYTHONPATH:/loom/python/"
