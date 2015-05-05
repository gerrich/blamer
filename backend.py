#!/usr/bin/env python
# -*- coding: utf8 -*-

import socket 
import re

def filter_records(shingles):
  result = []
  for shingle in shingles:
    if shingle['line'] == '' or re.match(r'^\s*[\{\}\;]?\s*$', shingle['line']) or shingle['line'][:2] == '//':
      continue
    result.append(shingle)
  return result

def ask_shingles(records):
  return ask_backend("find", [r['value'] for r in records])

def ask_audit(contest_id, keys):
  keys = ["%d"%int(k) for k in keys if re.match(r'^\d+$', k)]
  return ask_backend("audit", [contest_id] + keys)
  #return ask_backend("find", ["%06d"%(int(key),) for key in keys])

def ask_backend(cmd, keys):
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect(("127.0.0.1", 8001))
  cmd = cmd + " " + " ".join(keys)
  s.send(cmd + "\n")
  BUFFER_SIZE = 1024
  data = ''
  for i in range(10000):
    chunk = s.recv(BUFFER_SIZE)
    data += chunk
    if not chunk or len(chunk) == 0:
      break
  s.close()
  return data



def parse_tsv_response(data, shingles, sources):
  freq_dict = {}
  src_dict = {}
  for line in data.split("\n"):
    fields = line.split("\t")
    if len(fields) < 3:
      continue
    if not freq_dict.has_key(fields[0]):
      freq_dict[fields[0]] = 0;
    freq_dict[fields[0]] += 1
    src = ":".join(fields[1:3])
    if not src_dict.has_key(src):
      src_dict[src] = 0
    src_dict[src] += 1
  
  for pair in sorted(src_dict.items(), key=lambda x:-x[1]):
    sources.append(pair[0].split(":") + [pair[1]])
  
  for shingle in shingles:
    if freq_dict.has_key(shingle['value']):
      shingle['tags'] = ['1'] * freq_dict[shingle['value']]
  
def parse_json_response(data):
  return []


def process_audit_response(data, sources):
  src_dict = {}
  for line in data.split("\n"):
    fields = line.split("\t")
    try:
      src_dict[fields[0]] = {"runid":fields[0], 'status':fields[1], 'login':fields[2], 'uid':fields[3], "date":fields[4]} 
    except:
      pass

  for source in sources:
    if re.match(r'^\d+$', source['runid']):
      key = "%d"%(int(source['runid']),)
      if src_dict.has_key(key):
        #source.append(src_dict[key])
        source.update(dict(source.items() + src_dict[key].items()))
    
