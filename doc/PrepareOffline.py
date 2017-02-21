#!/usr/bin/python

import sys,os,re

if __name__=="__main__":
    linkSubstRe=re.compile(r'([^\\!]|^)(\[[^[\]()]*\]\((?!https?://|\\.)[^[\]()]*)\)')
    
    fi = sys.stdin
    fo = sys.stdout
    inQuote=False
    for l in fi:
        if l=='```\n':
            # print "quotechange"
            inQuote=not inQuote
            fo.write(l)
            continue
        # print "quote:",inQuote
        if not inQuote:
            newl = ''
            qpos = 0
            while qpos!=-1:
                nq = l.find('`',qpos)
                # print qpos,nq
                if nq!=-1 and (nq==0 or l[nq-1]!='\\'):
                    fo.write(linkSubstRe.sub(r'\1\2.html)',l[qpos:nq]))
                    qpos = l.find('`',nq+1)
                    if qpos!=-1:
                        fo.write('`'+l[nq+1:qpos]+'`')
                        qpos = qpos+1
                    else:
                        fo.write('`'+l[nq+1:-1]+'`\n')
                else:
                    fo.write(linkSubstRe.sub(r'\1\2.html)',l[qpos:]))
                    qpos = -1
        else:
            fo.write(l)
        