To install & start the payload, run the `sudo ./payload_setup.sh` script. 

To view the logs check `~/logs`

To start the payload, simply run `sudo systemctl start payload.service`

To stop the payload, simply run `./stop_payload.sh` or run:
``` 
sudo systemctl stop payload.service
rm /opt/Payload/.running.txt
```