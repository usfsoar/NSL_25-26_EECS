FROM python:3.12
LABEL author="l42aro"
WORKDIR /app
COPY ./requirements.txt requirements.txt
RUN pip install -r requirements.txt
COPY . .
ENTRYPOINT [ "python3", "app.py" ]
# CMD ["app.py"]
CMD []