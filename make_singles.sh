#!/bin/bash

export DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# shingle file format:
# <nash_sum> <line_num> <line> <file_id>

contest_id=000026
export find_path=/home/ejudge/ejudge-home/judges/${contest_id}/var/archive/runs
export audit_path=/home/ejudge/ejudge-home/judges/${contest_id}/var/archive/audit
export shingle_dir=/home/ejudge/blamer/shingles/${contest_id}

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
#(cd ${find_path} && find . -type f -printf "%P\n") |\
#xargs -I{} -P10 bash -c 'process_file {}'

#exit 0
append_file_tag () {
  file="$1"
  basename=$(basename $file)
  cat "${file}" | perl -lne 'print "$_\t'$basename'"'
}

export -f append_file_tag
#find "${shingle_dir}" -type f | xargs -I{} bash -c 'append_file_tag {}' |\
#LC_ALL=C sort -t'	' -k1,1 > ${shingle_dir}.all.txt


find $audit_path -type f |\
xargs -I{} bash -c "${DIR}/process_audit.pl < {}" |\
LC_ALL=C sort -S2G -t'	' -k1,1 > ${shingle_dir}.audit.all.txt

