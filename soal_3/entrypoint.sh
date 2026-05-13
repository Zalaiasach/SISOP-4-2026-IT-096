#!/bin/bash

if [ "$1" = "logger" ]; then
    LOG_FILE="/var/log/samba/libraryit.log"
    
    mkdir -p /var/log/samba
    touch "$LOG_FILE"
    
    tail -F "$LOG_FILE" | while read -r line; do
        if echo "$line" | grep -q "smbd_audit:"; then
            DATA=$(echo "$line" | awk -F'smbd_audit: ' '{print $2}')
            USER=$(echo "$DATA" | cut -d'|' -f1)
            SHARE=$(echo "$DATA" | cut -d'|' -f3)
            ACTION=$(echo "$DATA" | cut -d'|' -f4)
            STATUS=$(echo "$DATA" | cut -d'|' -f5)
            
            LEVEL="INFO"
            AKSI=$(echo "$ACTION" | tr '[:lower:]' '[:upper:]')
            
            if [[ "$STATUS" == "fail" || "$STATUS" == "failed" ]]; then
                LEVEL="WARNING"
                AKSI="DENIED"
            elif [[ "$ACTION" == "connect" ]]; then
                AKSI="CONNECT"
            fi
            
            NOW=$(date +"%Y-%m-%d %H:%M:%S")
            echo "[$NOW] [$LEVEL] [$USER] [$AKSI] [$SHARE]"
        fi
    done
    exit 0
fi

mkdir -p /var/log/samba
touch /var/log/samba/libraryit.log
echo "local7.* /var/log/samba/libraryit.log" > /etc/rsyslog.d/samba-audit.conf
service rsyslog start

groupadd readonly || true
groupadd staff || true

useradd -M -s /usr/sbin/nologin -g readonly member || true
useradd -M -s /usr/sbin/nologin -g staff contributor || true
useradd -M -s /usr/sbin/nologin -g staff librarian || true

(echo "member123"; echo "member123") | smbpasswd -a -s member
(echo "contrib456"; echo "contrib456") | smbpasswd -a -s contributor
(echo "lib789"; echo "lib789") | smbpasswd -a -s librarian

mkdir -p /libraryit/ebooks
mkdir -p /libraryit/papers
mkdir -p /libraryit/sourcecode
mkdir -p /libraryit/docs

chown -R root:staff /libraryit/ebooks /libraryit/papers
chmod -R 775 /libraryit/ebooks /libraryit/papers

chown -R root:staff /libraryit/sourcecode
chmod -R 750 /libraryit/sourcecode

chown -R librarian:staff /libraryit/docs
chmod -R 755 /libraryit/docs

exec smbd -F --no-process-group