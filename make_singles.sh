#!/bin/bash

export DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# shingle file format:
# <nash_sum> <line_num> <line> <file_id>

export find_path=/home/ejudge/ejudge-home/judges/000024/var/archive/runs
export shingle_dir=/home/ejudge/blamer/shingles/000024

process_file () {
  rel_path="$1"
  file="${find_path}/${rel_path}"
  echo $rel_path
  echo $file
  [ -f "$file" ] || return 1
  shingle_file=${shingle_dir}/${rel_path}
  [[ $shingle_file =~ \.gz$ ]] && shingle_file=${shingle_file%.*}

  mkdir -p $(dirname $shingle_file)
  [ -f $shingle_file ] && [[ $shingle_file -nt $file ]] && return 0

  {
  if [[ "$file" =~ \.gz$ ]]; then
    zcat "$file" 
  else
    cat "$file" 
  fi 
  } | python ${DIR}/shingle.py | LC_ALL=C sort -t '	' -k1,1 > ${shingle_file} 
  echo OK
}

export -f process_file

(cd ${find_path} && find . -type f -printf "%P\n") |\
xargs -I{} -P10 bash -c 'process_file {}'

exit 0
find "${shingle_dir}" -type f | xargs -I{} cat {} |\
LC_ALL=C sort -t'	' -k1,1 > ${shingle_dir}.all.txt


