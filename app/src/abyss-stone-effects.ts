import type {AdvancedItem,StatEdit} from './advanced-types';
import {plainText} from './format';
import type {Item} from './types';

export type StoneDetail = AdvancedItem['item'] & {name:string;description:string};
export interface StoneBonus {label:string;value:string}

// Only the five status fields observed on socket stones in the checked build.
// Flat attack/defence are stored in thousandths; per_level is an actual level
// delta, never a percentage. Unknown fields retain an explicitly raw unit.
const flatNames:Record<number,string>={1000002:'Angriff',1000003:'Verteidigung'};
const levelNames:Record<number,string>={1000007:'Kritische Trefferchance',1000010:'Angriffsgeschwindigkeit',1000011:'Bewegungsgeschwindigkeit'};
const buffEffects:Record<number,string>={
  1000096:'Verringert erlittenen Schaden',1000097:'Verbessert das Blocken',
  1000100:'Erhöht die Klettergeschwindigkeit',1000107:'Erhöht die Schwimmgeschwindigkeit',
  1000141:'Verändert den Ausdauerverbrauch',1000008:'Regeneriert Gesundheit',1000009:'Regeneriert Willenskraft',
  1000090:'Mehr Schaden gegen Maschinen',1000108:'Mehr Schaden gegen Hexen',1000109:'Mehr Schaden gegen Humanoide',
  1000110:'Mehr Schaden gegen Golems',1000111:'Mehr Schaden gegen Bosse',1000112:'Mehr Schaden gegen Tiere',1000113:'Mehr Schaden gegen Kreaturen',
  1000147:'Mehr kritische Treffer gegen Plattenrüstung',1000154:'Mehr kritische Treffer gegen Lederrüstung',1000157:'Mehr kritische Treffer gegen Stoffrüstung',
  1000153:'Verbessert das Schlachten',1000066:'Erhöht die Chance auf Ausrüstungsbeute',1000117:'Erhöht die Geldbeute',
  1000116:'Chance, beim Schießen keine Munition zu verbrauchen',1000089:'Verbessert das Kochen',1000091:'Verringert benötigte Herstellungsmaterialien',
  1000119:'Verbessert das Stehlen',1000071:'Mehr Ertrag beim Erzabbau',1000072:'Mehr Ertrag beim Pflanzensammeln',
  1000073:'Mehr Ertrag beim Sammeln von Tieren',1000093:'Mehr Ertrag beim Holzfällen',
  1000114:'Mehr Beitragserfahrung',1000115:'Mehr Fähigkeitenerfahrung',1000099:'Zusätzliche Freundschaft',
  1000123:'Zusätzliche Erfahrung für Reittiere',1000124:'Zusätzliche Zuneigung bei Haustieren',
  1000252:'Gesundheit bei Angriffen wiederherstellen',1000262:'Willenskraft bei Angriffen wiederherstellen',1000273:'Ausdauer bei Angriffen wiederherstellen',
  1000212:'Verbessert Angriffskombinationen',1000149:'Immunität gegen Abyss-Gras',1000150:'Immunität gegen Hyänengift',1000151:'Immunität gegen Bismut',
  1000152:'Erhöht den Schaden von Drehschlägen',
};
function signed(value:number){return `${value>0?'+':''}${value.toLocaleString('de-DE',{maximumFractionDigits:3})}`;}
function statBonus(s:StatEdit):StoneBonus|null{
  const n=Number(s.value);if(!Number.isFinite(n)||n===0)return null;
  if(s.list==='static'&&flatNames[s.stat])return {label:flatNames[s.stat],value:signed(n/1000)};
  if(s.list==='per_level'&&levelNames[s.stat])return {label:levelNames[s.stat],value:`${signed(n)} ${Math.abs(n)===1?'Stufe':'Stufen'}`};
  return {label:`Stat-ID ${s.stat} (${s.list})`,value:`${signed(n)} Rohwert`};
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
  return [...new Set(buffs.map(b=>buffEffects[b.buff]?`${buffEffects[b.buff]} · Effektstufe ${b.level}`:ability||`Spezialeffekt · Effektstufe ${b.level} (Buff-ID ${b.buff})`))];
}
