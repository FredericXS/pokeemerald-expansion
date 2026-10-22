#!/usr/bin/env python3
"""Validate the playable routes in actual generated HGSS-based GBA block data."""
import json
import struct
from collections import deque
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def load_json(path):return json.loads((ROOT/path).read_text())
LAYOUTS = {x['id']:x for x in load_json('data/layouts/layouts.json')['layouts']}
PRIMARY = struct.unpack('<640I',(ROOT/'data/tilesets/primary/general_frlg/metatile_attributes.bin').read_bytes())
SECONDARY_RAW = (ROOT/'data/tilesets/secondary/johto/metatile_attributes.bin').read_bytes()
SECONDARY = struct.unpack('<'+'I'*(len(SECONDARY_RAW)//4),SECONDARY_RAW)

def check(condition,message):
    if not condition:raise AssertionError(message)

class Grid:
    def __init__(self,name):
        self.name=name;self.map=load_json(f'data/maps/{name}/map.json');layout=LAYOUTS[self.map['layout']]
        self.w=layout['width'];self.h=layout['height'];data=(ROOT/layout['blockdata_filepath']).read_bytes()
        check(len(data)==self.w*self.h*2,f'{name}: incorrect block count')
        self.blocks=struct.unpack('<'+'H'*(self.w*self.h),data)
        self.actors={(o['x'],o['y']) for o in self.map['object_events']}
    def block(self,p):
        x,y=p
        return self.blocks[y*self.w+x] if 0<=x<self.w and 0<=y<self.h else 0xfc00
    def behavior(self,p):
        mid=self.block(p)&1023
        return (PRIMARY[mid] if mid<640 else SECONDARY[mid-640])&511
    def walkable(self,p):return not self.block(p)&0xc00 and self.block(p)>>12!=1 and p not in self.actors
    def reach(self,start):
        check(self.walkable(start),f'{self.name}: invalid player start {start}')
        visited={start};queue=deque([start])
        while queue:
            x,y=queue.popleft()
            for dx,dy,ledge in [(1,0,0x38),(-1,0,0x39),(0,1,0x3b),(0,-1,0x3a)]:
                p=(x+dx,y+dy)
                # Ledge jumps cross a blocked metatile in the allowed direction.
                if self.behavior(p)==ledge:p=(x+2*dx,y+2*dy)
                if p not in visited and self.walkable(p):visited.add(p);queue.append(p)
        return visited

def validate():
    town=Grid('NewBarkTown');route=Grid('JohtoRoute29')
    check((town.w,town.h)==(32,32),'HGSS New Bark must occupy its own 32x32 matrix cell')
    check((route.w,route.h)==(96,32),'HGSS Route 29 must run west/east across three matrix cells')
    check(town.map['connections']==[{'map':'MAP_JOHTO_ROUTE29','offset':0,'direction':'left'}],'New Bark west connection')
    check(route.map['connections']==[{'map':'MAP_NEW_BARK_TOWN','offset':0,'direction':'right'}],'Route 29 east connection')
    doors=[(12,9),(23,12),(7,21),(18,23),(16,8)]
    check({(w['x'],w['y']) for w in town.map['warp_events']}==set(doors),'HGSS door locations changed')
    reached=town.reach((3,14))
    for x,y in doors:
        check(town.behavior((x,y))==0x60,f'{(x,y)}: door lacks a working warp behavior')
        check((x,y+1) in reached,f'{(x,y)}: door approach is unreachable')
        check((x,y) in reached,f'{(x,y)}: doorway is blocked')
    for w in town.map['warp_events']:
        destination=next(p for p in (ROOT/'data/maps').glob('*/map.json') if json.loads(p.read_text())['id']==w['dest_map'])
        dest=json.loads(destination.read_text());back=dest['warp_events'][int(w['dest_warp_id'])]
        check(back['dest_map']=='MAP_NEW_BARK_TOWN',f'{destination}: exit leaves Johto')
        check(town.map['warp_events'][int(back['dest_warp_id'])]['dest_map']==dest['id'],f'{destination}: nonreciprocal exit')
    east=(93,14);west=(4,14)
    check(west in route.reach(east),'Route 29 has no east-to-west walking path')
    check(east in route.reach(west),'Route 29 ledges leave no west-to-east return path')
    check((50,6) in route.reach(east),'Route 46 gate is inaccessible')
    check(route.behavior((50,5))==0x60 and route.behavior((51,5))==0x60,'Gate doors do not warp')
    for grid in [town,route]:
        for actor in grid.map['object_events']:
            p=(actor['x'],actor['y'])
            check(not grid.block(p)&0xc00,f'{grid.name}: NPC stands inside an obstacle at {p}')
            check(any((p[0]+dx,p[1]+dy) in grid.reach((3,14) if grid is town else east) for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)]),f'{grid.name}: NPC cannot be approached at {p}')
    check(all(route.block((0,y))&0xc00 for y in range(12,19)),'Unfinished Cherrygrove boundary is open')
    check(all(town.block((x,y))>>12==1 for y in range(14,19) for x in range(29,32)),'New Bark eastern shore is missing')
    check(sum(route.behavior((x,y))==2 for y in range(route.h) for x in range(route.w))>=300,'Route 29 encounter grass was lost')
    for grid in [town,route]:
        check(grid.map['layout'] not in ['LAYOUT_ROUTE1','LAYOUT_PALLET_TOWN'],'Kanto layout alias remains')
    check(len((ROOT/'data/tilesets/secondary/johto/metatiles.bin').read_bytes())<=384*16,'Secondary metatile budget exceeded')
    print('PASS: HGSS geography, doors, reciprocal warps, NPC access, both walking directions, gate, shoreline, grass and tile budget')

if __name__=='__main__':validate()
