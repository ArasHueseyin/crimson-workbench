export function grantQuantity(raw:string):number|null {
  const text=raw.trim();if(!/^\d+$/.test(text))return null;
  const n=Number(text);return Number.isSafeInteger(n)&&n>=1&&n<=10000?n:null;
}
export function maximumNewStacks(quantity:number|null,stack:string):number|null {
  if(quantity===null)return null;try{const limit=BigInt(stack);if(limit<1n)return null;return Number((BigInt(quantity)+limit-1n)/limit);}catch{return null;}
}
