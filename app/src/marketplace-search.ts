const aliases:Record<string,string>={
 larger_lion:'Löwe größer groesser lion size',larger_rokade:'Rokade größer groesser horse size',
 faster_mounts:'Reittiere schneller Geschwindigkeit mounts speed',no_mount_stamina:'Reittiere Ausdauer unendlich stamina',
 inventory_999:'Inventar Taschen Plätze Plaetze inventory slots',no_durability_loss:'Haltbarkeit Verschleiß Verschleiss durability',
 region_return_30min:'Abgesperrte Regionen Rückkehr Rueckkehr Timer Grenze 30 Minuten',automatic_fishing:'Automatisch Angeln Fischen fishing',
 free_upgrade_materials:'Kostenlos Verbessern Aufwerten Materialien upgrades',free_crafting_currency:'Herstellen Insekten Kosten Geld crafting fee',
 skills:'Fähigkeiten Faehigkeiten freischalten alle skills abilities',item_stacks:'Stapel Stapeltiefe Stapelgröße Stapelgroesse items stack size',
 map:'Karte Nebel aufdecken world map fog',contributions:'Beitragswerte Beitragspunkte contribution faction points',
 effects:'Elementarwaffen Waffen Effekte Blitz Feuer Donner lightning fire',teleport:'Teleportieren Position Wegpunkt Karte reisen',
 photo_mode:'Fotomodus Kamera HUD Szene einfrieren photo camera',world_controls:'Wetter Uhrzeit Tageszeit Zeit Regen Schnee Wolken Wind Nebel weather time rain snow clouds fog',
 auto_loot:'Beute automatisch aufsammeln loot sammeln',mount_dismount:'Reittier Absteigen Sturz fallen Löwe Rokade',
 force_palm:'Kraftfaust Krauftfaust unbegrenzte Sprünge Spruenge aerial force palm',
 force_palm_height:'Kraftfaust Krauftfaust Sprunghöhe Sprunghoehe höher hoeher 100x force palm jump height',
 free_crafting_materials:'Herstellen Materialien kostenlos crafting',mount_protection:'Reittiere Schaden schützen schuetzen unsterblich mount protection',
 preserve_consumables:'Verbrauchskosten Verbrauchsgegenstände Verbrauchsgegenstaende Pfeile Tränke Traenke Essen arrows potions food',
 player_speed:'Spieler Bewegung Geschwindigkeit schneller movement speed',player_jump:'Spieler Sprunghöhe Sprunghoehe springen jump height',
 mount_jump:'Reittiere höher hoeher springen Sprunghöhe Sprunghoehe Löwe Rokade',
 player_stamina:'Spieler Ausdauer Geist unendlich unbegrenzt unlimited spirit stamina MP resources',
 axiom_reach:'Axiom Armreif Armband Arm Reichweite Länge Laenge greifen reach range length force',
 glide_speed:'Gleiten Gleitgeschwindigkeit schneller fliegen glide gliding flight speed 500%',
 no_fall:'Kein Fallschaden Sturzschaden fall damage',sleep_cooldown:'Schlafen Wartezeit Pause cooldown',
 sleep_duration:'Schlafen Schlafdauer 30 Stunden sleep',blackstar:'Schwarzstern Drache überall ueberall fliegen Blackstar',
};
function normalise(value:string){return value.normalize('NFKD').replace(/[\u0300-\u036f]/g,'').toLowerCase().replace(/ß/g,'ss').replace(/×/g,'x')}
export function matchesMarketplaceMod(query:string,id:string,...text:string[]):boolean {
 const words=normalise(query).trim().split(/\s+/).filter(Boolean);
 const searchable=normalise([id.replaceAll('_',' '),aliases[id]??'',...text].join(' '));
 return words.every(word=>searchable.includes(word));
}
