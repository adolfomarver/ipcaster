from ipcaster import ipcaster
import requests
import json

service = ipcaster('http://localhost:8080')
active_streams = list()
media_files_dir = '../tsfiles/'

def test_createStream():
    # create a new stream
    resp = service.createStream(media_files_dir + 'test.ts', '172.17.0.1', 50000)
    assert resp.status_code == 200
    active_streams.append(resp.json())

def test_listStreams():
    # check the stream is in the list of running streams
    resp = service.listStreams()
    assert resp.status_code == 200
    found = False
    for s in resp.json()['streams']:
        if s['id'] == active_streams[0]['id']:
            found = True
            break
    assert found

def test_deleteStream():
    # delete the stream
    resp = service.deleteStream(active_streams.pop()['id'])
    assert resp.status_code == 200

def test_deleteStreamError():
    # delete a stream that doesn't exist
    resp = service.deleteStream(666)
    assert resp.status_code == 400


    

    
    

        
