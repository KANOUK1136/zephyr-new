from flask import Flask, request, jsonify
import json
import requests

app = Flask(__name__)

@app.route("/api/home", methods=['GET'])
def hello_world():
    return ("Bienvenue !!!   ")

@app.route("/api/transfer", methods=['POST', 'GET'])
def valtemp():
    data = request.json
    print(data)
    return jsonify({"Reponse": "OK"})

if __name__ == '__main__':
    app.run(host= "127.0.0.1")