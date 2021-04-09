SRCD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $SRCD && gunzip $SRCD/full_mpeg4.db.gz 
cd $SRCD && ../../bin/most -p -f mpeg_optimizer_rsmc.scr -T 
