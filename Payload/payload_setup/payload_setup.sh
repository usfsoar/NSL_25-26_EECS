mkdir -p /opt/Payload
cp -r ../* /opt/Payload
rm -f /opt/Payload/.running.txt
chmod -R 777 /opt/Payload
chmod 777 /opt/Payload/payload_main.py

cp payload.service /etc/systemd/system/
chmod 644 /etc/systemd/system/payload.service

systemctl daemon-reload

# Make payload start on startup
systemctl enable payload.service

# Start payload program without restart
systemctl start payload.service


mkdir -p /opt/Payload/logs

for dir in /home/*; do 
    [ -d $dir ] || continue
    ln -sfn /opt/Payload/ "$dir/payload-files"
    ln -sfn /opt/Payload/logs "$dir/logs"
done

echo "Done!"