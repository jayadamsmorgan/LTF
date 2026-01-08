# LTF SSH Example

- Create `.secrets` file and populate with SSH user/password credentials

```
user=your_username
password=your_password
```

- Now if you have entered credentials for the user on your current machine and you have SSH enabled, you can run:

```bash
ltf test
```

- If you want to connect to any other SSH host:

```bash
ltf test -v ip=your_custom_ip,port=your_custom_ssh_port
```
