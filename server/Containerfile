FROM python:alpine3.21

ARG LPORT=1911
ARG WORKDIR=/server

RUN addgroup --system appgroup && adduser --system --ingroup appgroup appuser

RUN apk add --no-cache ffmpeg

WORKDIR ${WORKDIR}

ENV VIRTUAL_ENV=${WORKDIR}/venv
ENV PATH="${VIRTUAL_ENV}/bin:$PATH"

RUN python -m venv $VIRTUAL_ENV

COPY requirement.txt .

RUN apk add --no-cache --virtual .build-deps \
        build-base \
        python3-dev \
        libffi-dev && \
    pip install --upgrade pip && \
    pip install --no-cache-dir -r requirement.txt && \
    apk del .build-deps

COPY . .

RUN chown -R appuser:appgroup ${WORKDIR}

EXPOSE ${LPORT}

USER appuser

CMD ["python", "server.py"]
