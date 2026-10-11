import type {AdvancedItem,StatEdit} from './advanced-types';
import {plainText} from './format';
import type {Item} from './types';

export type StoneDetail = AdvancedItem['item'] & {name:string;description:string};
export interface StoneBonus {label:string;value:string}

// Only the five status fields observed on socket stones in the checked build.
// Flat attack/defence are stored in thousandths; per_level is an actual level
// delta, never a percentage. Unknown fields retain an explicitly raw unit.
const flatNames:Record<number,string>={1000002:'Attack',1000003:'Defense'};
const levelNames:Record<number,string>={1000007:'Critical hit chance',1000010:'Attack speed',1000011:'Movement speed'};
const buffEffects:Record<number,string>={
  1000096:'Reduces damage taken',1000097:'Improves blocking',
  1000100:'Increases climbing speed',1000107:'Increases swimming speed',
  1000141:'Changes stamina consumption',1000008:'Restores health',1000009:'Restores spirit',
  1000090:'More damage against machines',1000108:'More damage against witches',1000109:'More damage against humanoids',
  1000110:'More damage against golems',1000111:'More damage against bosses',1000112:'More damage against animals',1000113:'More damage against creatures',
  1000147:'More critical hits against plate armor',1000154:'More critical hits against leather armor',1000157:'More critical hits against cloth armor',
  1000153:'Improves butchering',1000066:'Increases equipment drop chance',1000117:'Increases money drops',
  1000116:'Chance to fire without consuming ammunition',1000089:'Improves cooking',1000091:'Reduces crafting material requirements',
  1000119:'Improves stealing',1000071:'More ore from mining',1000072:'More plants from gathering',
  1000073:'More materials from animals',1000093:'More wood from logging',
  1000114:'More contribution experience',1000115:'More skill experience',1000099:'Additional friendship',
  1000123:'Additional mount experience',1000124:'Additional pet affection',
  1000252:'Restore health when attacking',1000262:'Restore spirit when attacking',1000273:'Restore stamina when attacking',
  1000212:'Improves attack combinations',1000149:'Immunity to Abyss grass',1000150:'Immunity to hyena poison',1000151:'Immunity to bismuth',
  1000152:'Increases spin attack damage',
};
function signed(value:number){return `${value>0?'+':''}${value.toLocaleString('en-US',{maximumFractionDigits:3})}`;}
function statBonus(s:StatEdit):StoneBonus|null{
  const n=Number(s.value);if(!Number.isFinite(n)||n===0)return null;
  if(s.list==='static'&&flatNames[s.stat])return {label:flatNames[s.stat],value:signed(n/1000)};
  if(s.list==='per_level'&&levelNames[s.stat])return {label:levelNames[s.stat],value:`${signed(n)} ${Math.abs(n)===1?'level':'levels'}`};
  return {label:`Stat ID ${s.stat} (${s.list})`,value:`${signed(n)} Raw value`};
}
export function stoneDescription(item:Pick<Item,'description'>){return plainText(item.description.replace(/<br\s*\/?\s*>/gi,'\n'));}
export function stoneBonuses(detail:StoneDetail|undefined):StoneBonus[]{
  if(!detail)return [];
  const level=detail.enchant_levels.includes(0)?0:detail.enchant_levels[0];
  return detail.stats.filter(s=>s.enchant_level===level).map(statBonus).filter((s):s is StoneBonus=>s!==null);
}
export function stoneEffects(item:Pick<Item,'description'>,detail:StoneDetail|undefined):string[]{
  if(!detail)return [];
  const level=detail.enchant_levels.includes(0)?0:detail.enchant_levels[0];
  const buffs=detail.buffs.filter(b=>b.enchant_level===level);
  // Ability descriptions come from the selected game's localized ItemInfo;
  // numeric buff levels are strength tiers, not guessed % values.
  const ability=stoneDescription(item).split(/\n\s*\n/).slice(1).join('\n\n').trim();
  return [...new Set(buffs.map(b=>buffEffects[b.buff]?`${buffEffects[b.buff]} · Effect level ${b.level}`:ability||`Special effect · Effect level ${b.level} (Buff ID ${b.buff})`))];
}
