SRCD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $SRCD && gunzip $SRCD/full_mpeg4.db.gz 
cd $SRCD && ../../bin/most -f mpeg_optimizer.scr -T 
 
