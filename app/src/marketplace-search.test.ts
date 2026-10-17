import {describe,expect,it} from 'vitest';
import {matchesMarketplaceMod as matches} from './marketplace-search';
describe('marketplace mod search',()=>{
 it('matches names, effects and categories regardless of case or word order',()=>{
  expect(matches('  STAMINA mount  ','no_mount_stamina','No mount stamina cost','STAMINA')).toBe(true);
  expect(matches('weather fog','world_controls','Live time & weather')).toBe(true);
  expect(matches('weather fishing','automatic_fishing','Automatic fishing')).toBe(false);
  expect(matches('   ','blackstar')).toBe(true);
 });
 it('finds English cards using German terminology, umlauts and the requested spelling',()=>{
  expect(matches('LÖWE grösser','larger_lion','Larger lion')).toBe(true);
  expect(matches('schwarzstern überall','blackstar','Blackstar anywhere')).toBe(true);
  expect(matches('krauftfaust höhe','force_palm_height','Force Palm jump height')).toBe(true);
  expect(matches('axiom länge','axiom_reach','Axiom armlet reach')).toBe(true);
  expect(matches('geist','player_stamina','Unlimited player stamina & spirit')).toBe(true);
  expect(matches('GRÖẞER','larger_lion','Larger lion')).toBe(true);
  expect(matches('5x','glide_speed','Gliding speed 5×')).toBe(true);
 });
});
