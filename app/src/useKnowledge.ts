import {useCallback,useEffect,useMemo,useRef,useState} from 'react';
import {api,asError} from './api';
import type {KnowledgeSnapshot} from './knowledge-types';
export function useKnowledge(session:number) {
  const [data,setData]=useState<KnowledgeSnapshot|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState('');
  const generation=useRef(0);
  const refresh=useCallback(async(save:string|null=null)=>{
    const token=++generation.current;setBusy(true);setError('');setData(null);
    try {const value=await api.knowledge(session,save);if(token===generation.current)setData(value);}
    catch(e){if(token===generation.current)setError(asError(e).message);}
    finally {if(token===generation.current)setBusy(false);}
  },[session]);
  useEffect(()=>{void refresh();return()=>{generation.current++;};},[refresh]);
  const byItem=useMemo(()=>new Map(data?.items.map(i=>[i.key,i])??[]),[data]);
  return {data,busy,error,refresh,byItem};
}
