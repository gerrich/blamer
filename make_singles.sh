#!/bin/bash

export DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# shingle file format:
# <nash_sum> <line_num> <line> <file_id>

function get_find_path() {
  echo "/home/ejudge/ejudge-home/judges/$1/var/archive/runs"
}
function get_audit_path() {
  echo "/home/ejudge/ejudge-home/judges/$1/var/archive/audit"
}
function get_shingle_dir() {
  echo "/home/ejudge/blamer/shingles/$1"
}

export -f get_find_path
export -f get_audit_path
export -f get_shingle_dir

process_file () {
  local contest_id="$1"
  local rel_path="$2"
  local file="$(get_find_path ${contest_id})/${rel_path}"
  #echo $rel_path
  #echo $file
  [ -f "$file" ] || return 1
  local shingle_dir=$(get_shingle_dir ${contest_id}) 
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
  #echo OK
}

append_file_tag () {
  local file="$1"
  basename=$(basename $file)
  cat "${file}" | perl -lane 'print join "\t", $F[0], "'$basename'", $F[1]'
}

function process_contest() {
  local contest_id="$1"
  export -f process_file
  local find_path=$(get_find_path $contest_id)
  (cd ${find_path} && find . -type f -printf "%P\n") |\
  xargs -I{} -P10 bash -c 'process_file "'${contest_id}'" {}'

  #exit 0

  local shingle_dir=$(get_shingle_dir ${contest_id})
  local latest_shingle=$( find ${shingle_dir} -type f -printf '%T@ %p\n' | sort -n | tail -n1 | cut -f2- -d" " )
  [ ! -f ${shingle_dir}.all.txt ] || [[ ${shingle_dir}.all.txt -ot ${latest_shingle} ]] && { 
    export -f append_file_tag
    find ${shingle_dir} -type f | xargs -I{} bash -c 'append_file_tag {}' |\
    LC_ALL=C sort -t'	' -k1,1 > ${shingle_dir}.all.txt.new
  
    mv ${shingle_dir}.all.txt{,.bkp}
    mv ${shingle_dir}.all.txt{.new,}

    find $(get_audit_path $contest_id) -type f |\
    xargs -I{} bash -c "${DIR}/process_audit.pl < {}" |\
    LC_ALL=C sort -S2G -t'	' -k1,1 > ${shingle_dir}.audit.all.txt.new
  
    mv ${shingle_dir}.audit.all.txt{,.bkp}
    mv ${shingle_dir}.audit.all.txt{.new,}
  }
}

#for _id in 000027; do
for _id in 000023 000024 000026 000027; do
  process_contest "$_id"
done 

set -x
echo "reload" | nc localhost 8001
set -

