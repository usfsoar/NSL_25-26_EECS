# SOAR ECHO BASE 🚀📡

![GitHub top language](https://img.shields.io/github/languages/top/usfsoar/NSL_23-24_PayloadEECS.svg)

This is the project for the SOAR flight computer

![SOAR Echo base banner image](ECHO.png)

## Getting started

There are 2 ways to get started, an "Easy" way and the "Hard" way:

### Easy way

Download Docker in your Desktop. Create an account, say you're a student. Make sure to disable auto startup by going: Task Manager > Startup Apps, then disable docker's startup status

Once that is done you can either run the file start.ps1 on windows or runt he file start.sh on linux (make sure to run chmod +x start.sh first). And you should be up and running

### Hard way

The hard way is more strict however may allow for faster development, although still test your stuff using the Docker approach before deploying on the flight computer to be safe.

1. First download `python 3.12`
2. `cd` into SOAR_Echo_Base and run `python -m pip install -r requirements.txt`
3. Try to handle all exceptions that occur while downloading (python is annoying sometimes about it)
4. Run `app.py`
5. If errors occur due to missing libraries try to install them on your computer

**NOTE:** Pending to create a python venv or Conda environment to make this process more robust and cross platform (windows, linux, raspbian)
