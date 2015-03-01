import sys
import hashlib

# file is treated as 
def make_shingles(file):
  result = []
  line_number = 0
  for line in file:
    line = line.rstrip('\n')
    value = hashlib.md5(line).hexdigest()
    result.append({'line_number':line_number, 'value':value, 'line':line})
    line_number += 1
  return result

if __name__=='__main__':
  result = make_shingles(sys.stdin)
  for res in result:
    print "%s\t%s"%(res['value'], res['line'])
    
