export type WorldOperation='choose_time'|'advance_time'|'freeze_time'|'release_time'|'choose_weather'|'freeze_weather'|'release_weather';
export type WorldState='unavailable'|'ready'|'queued'|'published'|'rejected'|'expired'|'world_changed'|'uncertain'|'fault_latched'|'released';
export type TimeChoice={hour:number;minute:number;second:number;millisecond:number};
export type WeatherChoice={precipitation:number;cloudiness:number;humidity:number;wind_speed:number;height_fog:number;phase:number};
export type WorldSnapshot={
 pid:number;state:WorldState;id:string;session:string;generation:string;monotonic_ms:string;
 reason:number;flags:number;identity:number;world:number;stage:number;clock_reason:number;weather_reason:number;
 time:TimeChoice&{day:number;weekday:number;rate:number;anchor:string};weather:WeatherChoice;
 time_receipt:string;weather_receipt:string;
 time_result:WorldState;time_result_reason:number;weather_result:WorldState;weather_result_reason:number;
};
export type WorldChoice=
 |{operation:'choose_time';time:TimeChoice}
 |{operation:'advance_time';hours:number}
 |{operation:'choose_weather';weather:WeatherChoice}
 |{operation:'freeze_time'|'freeze_weather';enabled:boolean}
 |{operation:'release_time'|'release_weather'};
export type WorldRequest={pid:number;id:string;session:string;generation:string;not_after_ms:string;operation:WorldOperation;time:TimeChoice|null;weather:WeatherChoice|null;enabled:boolean|null;hours:number|null};
