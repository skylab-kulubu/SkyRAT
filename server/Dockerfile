# Multi-stage Dockerfile for SkyRAT Server
# Stage 1: Builder stage
FROM python:3.11-alpine AS builder

# Install build dependencies
RUN apk add --no-cache \
    build-base \
    python3-dev \
    libffi-dev \
    openssl-dev \
    cargo \
    rust

# Create virtual environment
RUN python -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Copy requirements first for better caching
COPY requirement.txt /tmp/
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r /tmp/requirement.txt

# Stage 2: Runtime stage
FROM python:3.11-alpine AS runtime

# Install runtime dependencies
RUN apk add --no-cache \
    ffmpeg \
    libffi \
    openssl \
    tini

# Create non-root user
RUN addgroup --system --gid 1001 appgroup && \
    adduser --system --uid 1001 --ingroup appgroup appuser

# Copy virtual environment from builder
COPY --from=builder /opt/venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Set working directory
WORKDIR /app

# Copy application files with proper ownership
COPY --chown=appuser:appgroup . .

# Create necessary directories
RUN mkdir -p /app/keys /app/recordings /app/logs && \
    chown -R appuser:appgroup /app

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD python -c "import socket; s=socket.socket(); s.connect(('localhost', int(os.environ.get('LPORT', 1911)))); s.close()" || exit 1

# Expose port
EXPOSE 1911

# Use tini as PID 1
ENTRYPOINT ["/sbin/tini", "--"]

# Switch to non-root user
USER appuser

# Default command
CMD ["python", "server.py"]