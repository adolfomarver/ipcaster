#
# Copyright (C) 2019 Adofo Martinez <adolfo at ipcaster dot net>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import requests

# Implements the IPCaster API methods 
class ipcaster:
    def __init__(self, ipcaster_url):
        self.url = ipcaster_url    

    # List the running streams 
    def listStreams(self):
        return requests.get(self.url+"/api/streams")

    # Creates a new stream
    def createStream(self, file_path, target_ip, target_port):
        headers = {'Content-Type': 'application/json'}
        body = f'{{"source" : "{file_path}", "endpoint": {{ "ip": "{target_ip}", "port": {target_port} }}  }}'
        return requests.request('POST', self.url+"/api/streams", headers=headers, data = body)

    # Deletes a stream currently running in the service by its id
    def deleteStream(self, stream_id):
        return requests.delete(self.url+"/api/streams/"+str(stream_id))

    
    

        
