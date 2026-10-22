#!/usr/bin/env python3
"""Export an auditable Portuguese Johto reconstruction reference from pret/pokeheartgold.

Usage: python3 tools/generations_maps/export_hgss_reference.py /path/to/pokeheartgold
The external source checkout is read only; normal ROM builds do not need it.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
COMMIT = '6988158691eb6178cff697cda0428045ddcba5fb'

def matrix(data):
    w,h,has_headers,has_heights,length = data[:5]
    pos = 5+length
    headers = struct.unpack_from(f'<{w*h}H',data,pos) if has_headers else [0]*(w*h)
    pos += 2*w*h if has_headers else 0
    heights = data[pos:pos+w*h] if has_heights else bytes(w*h)
    pos += w*h if has_heights else 0
    return w,h,headers,heights,struct.unpack_from(f'<{w*h}H',data,pos)

def narc(data):
    pos=16;sections={}
    for _ in range(struct.unpack_from('<H',data,14)[0]):
        tag=data[pos:pos+4];size=struct.unpack_from('<I',data,pos+4)[0]
        sections[tag]=pos;pos+=size
    fat=sections[b'BTAF'];base=sections[b'GMIF']+8
    return [data[base+a:base+b] for a,b in (struct.unpack_from('<II',data,fat+12+i*8) for i in range(struct.unpack_from('<H',data,fat+8)[0]))]

def generate(source):
    actual=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
    if actual != COMMIT:
        raise SystemExit(f'Expected reference commit {COMMIT}, got {actual}')
    definitions=(source/'include/constants/maps.h').read_text()
    ids={name:int(value) for name,value in re.findall(r'^#define (MAP_\w+)\s+(\d+)',definitions,re.M)}
    names={value:name for name,value in ids.items()}
    headers={name:dict(re.findall(r'\.(\w+) = ([\w]+)',body)) for name,body in re.findall(r'\[(MAP_\w+)\] = \{(.*?)\n\s*\},',(source/'src/data/map_headers.h').read_text(),re.S)}
    behavior_text=(source/'include/constants/metatile_behavior.h').read_text().split('enum TILE_BEHAVIOR {')[1].split('}')[0]
    behaviors=re.findall(r'^\s*(TILE_BEHAVIOR_\w+)\s*,',behavior_text,re.M)
    notes=json.loads((Path(__file__).with_name('area_notes.json')).read_text())
    matrix_path=source/'files/fielddata/mapmatrix/map_matrix/map_matrix_0000_EVERYWHERE.bin'
    w,h,owners,heights,models=matrix(matrix_path.read_bytes())
    land_path=source/'files/a/0/6/5';lands=narc(land_path.read_bytes())
    lines=[]
    def out(text=''):lines.append(text)
    out('JOHTO — REFERÊNCIA DE RECONSTRUÇÃO HG/SS, TILE A TILE')
    out('='*78)
    out('Escopo: dez cidades de Johto, Lago da Fúria, rotas 29–48, Safari Zone Gate')
    out('e Rota 28 como ligação complementar a Mt. Silver. Texto em UTF-8.')
    out('')
    out('FONTES E GRAU DE PRECISÃO')
    out('Fonte primária: https://github.com/pret/pokeheartgold/tree/'+COMMIT)
    out('Commit pesquisado: '+COMMIT)
    out('Mapas/eventos: src/data/map_headers.h; include/constants/maps.h;')
    out('files/fielddata/eventdata/zone_event/*.json; matriz exterior EVERYWHERE;')
    out('terrenos LAND do arquivo NARC files/a/0/6/5; enum metatile_behavior.h.')
    out('Instâncias de cenário: src/field/map_prop_manager.c (MapPropArcData).')
    out('SHA-256 da matriz: '+hashlib.sha256(matrix_path.read_bytes()).hexdigest())
    out('SHA-256 do NARC de terrenos: '+hashlib.sha256(land_path.read_bytes()).hexdigest())
    out('A imagem regional enviada pelo usuário orienta a topologia. Ela não fornece')
    out('a grade local nem escala métrica para montar os prédios. As coordenadas,')
    out('portas, objetos, condições e atributos abaixo vêm dos arquivos de HGSS.')
    out('As notas de composição são orientação artística para a adaptação a GBA.')
    out('')
    out('IMPORTANTE PARA UMA RECRIAÇÃO PRECISA: atributos de colisão não identificam')
    out('a textura exata de um pixel, cor do telhado ou contorno de uma copa. HGSS usa')
    out('terreno/modelos 3D e objetos de construção, não um único tileset GBA pronto.')
    out('Para fidelidade visual, conferir também os modelos/tex do setor no NARC LAND')
    out('e os prédios em files/fielddata/build_model/bm_field.narc. Não interpretar')
    out('todo bloqueio 0x8000 como árvore, nem toda superfície 0x0400 como estrada.')
    out('Esta referência não afirma reprodução visual pixel a pixel das áreas futuras.')
    out('')
    out('CONVENÇÕES DE COORDENADAS E LEITURA DOS TILES')
    out('- Norte = topo; leste = direita. x cresce para a direita; y para baixo.')
    out('- Cada célula da matriz mundial tem 32×32 posições de movimento. Na adaptação')
    out('  GBA, cada posição corresponde a um metatile de 16×16 px, formado por 8x8 px.')
    out('- HGSS chama o eixo vertical do chão de z. Nesta referência: y = z de HGSS.')
    out('- Coordenada local = coordenada mundial menos a origem indicada para a área.')
    out('- O retângulo envolvente pode incluir células de outra área; somente setores')
    out('  explicitamente listados pertencem ao cabeçalho pesquisado. Não preencher')
    out('  os espaços vazios do retângulo copiando um setor vizinho.')
    out('- Altura da matriz é registrada por setor; y dos eventos é altura vertical,')
    out('  não a linha da grade. Pontes, cavernas e escadas exigem conferir níveis.')
    out('- Linhas RLE: y=00 00-05:8006 significa posições x=0 até x=5 inclusive,')
    out('  todas com atributo hexadecimal 0x8006. Não são IDs de metatile da GBA.')
    out('- O byte inferior indexa TILE_BEHAVIOR; os demais bits são preservados crus.')
    out('  0x8000 é o bloqueio básico. Modificadores, eventos e diferenças de altura')
    out('  também influenciam acesso no jogo; não converter apenas este bit em toda')
    out('  a lógica de movimentação, especialmente em pontes e rotas montanhosas.')
    out('')
    out('DICIONÁRIO ARTÍSTICO PARA A MONTAGEM GBA')
    for text in [
        'G01 Gramado: base contínua, variações discretas de folhagem; sem colisão.',
        'G02 Mato alto: preenchimento de manchas definidas, comportamento de encontro; bordas não devem desenhar metade de uma árvore.',
        'G03 Árvore: montar copa, tronco e sombra como objeto completo; manter todas as peças na mesma âncora e bloquear o tronco. Não deixar quadrantes isolados.',
        'G04 Arbusto: objeto baixo independente para borda fina, nunca substituto automático de todos os prédios e rochedos do mapa.',
        'P01 Terra: centro, quatro lados, quatro cantos externos e cantos internos. A borda aparece somente onde muda a superfície.',
        'P02 Pavimento: piso urbano, calçada, esquinas e guia; alinhar a grade entre quarteirões e deixar as portas ligadas à rua.',
        'W01 Água aberta: tile sem faixa de margem; padrões suaves repetíveis nos quatro lados. Não repetir tile de costa em todo o lago.',
        'W02 Costa: lado N/S/E/O, cantos externos/internos e transição à terra ou areia; escolher por vizinhança de água e terra.',
        'W03 Água especial: cascata, corrente e redemoinho têm arte e comportamento próprios; não são água plana recolorida.',
        'R01 Falésia: topo caminhável, lábio, parede vertical, base e cantos. O corpo vertical fica bloqueado e não substitui o piso do patamar.',
        'R02 Salto: desenhar a borda no sentido permitido, preservar a área livre de aterrissagem. Não confundir ladder/down com jump/south.',
        'R03 Caverna: abertura escura embutida em rocha, base conectada ao patamar e warp no vão; evitar árvores cortando a boca.',
        'B01 Fachada: telhado, beiral, parede, janelas, embasamento e porta do MESMO prédio. Recortar sem pedaços de construções vizinhas.',
        'B02 Porta: localizar no vão real da fachada, criar aproximação livre e ligar warp/retorno. O jogador não deve entrar sob uma janela.',
        'B03 Escada exterior: degrau de partida, sequência contínua e patamar superior; o acesso não deve conter uma segunda escada sobreposta.',
        'B04 Ponte/cais: piso, corrimão, início/fim e suportes coerentes; passagem central livre e limites laterais bloqueados.',
        'I01 Estante: canto superior esquerdo/direito e bases correspondentes; não repetir duas peças esquerdas formando uma emenda falsa.',
        'I02 Mesa: tampo esquerdo/centro/direito e pés inferiores combinados. A colisão inclui o mobiliário; NPCs e bolas ficam em posição apropriada.',
        'I03 Interior: paredes com cantos corretos, piso contínuo e circulação entre entrada, NPC e serviços. Não usar cozinha em portão de rota.',
        'E01 Placa: base apoiada em terra, lado da porta, texto e interação consistentes. Não colocá-la no vão de uma porta.',
        'E02 NPC: posição inicial livre, orientação e alcance de movimento válidos. Garantir ao menos uma célula de aproximação para conversar.',
        'E03 Itens/Apricorn: objetos/eventos com estado de coleta, não plantas duplicadas na textura do chão.'
    ]:out(text)
    out('')
    out('ORDEM DE TRABALHO E REVISÃO VISUAL')
    out('1. Definir setores, alturas, ligações e portas antes de escolher decoração.')
    out('2. Montar solo/água/rocha; aplicar bordas por vizinhança, sem listras internas.')
    out('3. Ancorar cada fachada pela porta original; conferir largura e escadas.')
    out('4. Limpar árvores sob telhados e completar as árvores da borda do recorte.')
    out('5. Distribuir objetos segundo coordenadas/eventos, não de forma aleatória.')
    out('6. Conferir caminhos nos dois sentidos, saltos, colisão de mesa e retorno de portas.')
    out('7. Inspecionar a área inteira e capturas no emulador após a animação atualizar.')
    out('8. Testar transição, interior, saída e Continue com save anterior da ROM.')
    for map_name,note in notes.items():
        out('');out('='*78);out(note['name'].upper());out('='*78)
        for paragraph in note['notes']:out(paragraph)
        header=headers[map_name];map_id=ids[map_name]
        cells=[(i%w,i//w) for i,value in enumerate(owners) if value==map_id]
        if not cells:raise ValueError(f'No global matrix cells for {map_name}')
        x0=min(x for x,y in cells)*32;y0=min(y for x,y in cells)*32
        width=(max(x for x,y in cells)+1)*32-x0;height=(max(y for x,y in cells)+1)*32-y0
        out('');out(f'Cabeçalho: {map_name} (id {map_id}); matriz mundial: {w}×{h} setores.')
        out(f'Origem mundial: ({x0},{y0}); retângulo envolvente: {width}×{height} tiles.')
        event_name=header['eventsBank'].removeprefix('NARC_zone_event_').removesuffix('_bin')+'.json'
        event_path=source/'files/fielddata/eventdata/zone_event'/event_name
        events=json.loads(event_path.read_text())
        out('Eventos: files/fielddata/eventdata/zone_event/'+event_name)
        out('Banco de modelos/área: '+header.get('areaDataBank','?')+'; scripts: '+header.get('scriptsBank','?'))
        out('Portas e destinos (x,y locais; altura do evento; índice do retorno):')
        for index,event in enumerate(events.get('warps', [])):
            out(f"  {index}: ({event['x']-x0},{event['z']-y0}); altura={event['y']}; destino={event['header']}; anchor={event['anchor']}")
        if not events.get('warps', []):out('  Nenhum warp explícito; ligações exteriores ocorrem pela matriz.')
        out('Objetos originais (condições de visibilidade mantidas como símbolos HGSS):')
        for event in events.get('objects', []):
            out(f"  ({event['x']-x0},{event['z']-y0}) altura={event['y']} {event['spriteId']} id={event['id']}; direção={event['facingDirection']}; movimento={event['movement']}; raio=({event['xRange']},{event['yRange']}); flag={event['eventFlag']}; script={event['scriptId']}")
        out('Placas/interações de cenário:')
        for event in events.get('bgs', []):
            out(f"  ({event['x']-x0},{event['z']-y0}) altura={event['y']}; tipo={event['type']}; direção={event['dir']}; script={event['scriptId']}")
        out('Gatilhos de coordenada:')
        for event in events.get('coords', []):
            out(f"  ({event['x']-x0},{event['z']-y0}) área={event['w']}×{event['h']}; altura={event['y']}; {event['var']}={event['val']}; script={event['scriptId']}")
        out('Vizinhos imediatos por setor (inclui a própria área quando continua):')
        for x,y in cells:
            adjacent=[]
            for dx,dy,label in [(0,-1,'N'),(1,0,'E'),(0,1,'S'),(-1,0,'O')]:
                if 0<=x+dx<w and 0<=y+dy<h:
                    neighbor=owners[(y+dy)*w+x+dx];adjacent.append(label+'='+names.get(neighbor,str(neighbor)))
            out(f'  matriz({x},{y}): '+', '.join(adjacent))
        out('Nota: vizinhança de setor não garante uma passagem; conferir máscara/altura.')
        total=Counter();sections=[]
        for x,y in cells:
            i=y*w+x;mid=models[i];raw=lands[mid]
            sizes=struct.unpack_from('<4I',raw);offset=len(raw)-sum(sizes)
            if sizes[0]!=2048 or offset<16:raise ValueError(f'Unexpected LAND {mid}: {sizes}, offset {offset}')
            attrs=struct.unpack_from('<1024H',raw,offset)
            total.update(attrs);sections.append((x,y,mid,heights[i],attrs))
        out('Inventário exato de atributos usados (quantidade de tiles por valor):')
        for attr,count in sorted(total.items()):
            behavior=attr&255;label=behaviors[behavior] if behavior<len(behaviors) else 'NÃO_NOMEADO'
            out(f'  {attr:04X}: {count:5d}; comportamento {behavior:02X} {label}; bits superiores {attr&0xff00:04X}')
        for x,y,mid,height_,attrs in sections:
            raw=lands[mid];sizes=struct.unpack_from('<4I',raw);offset=len(raw)-sum(sizes)
            props=raw[offset+sizes[0]:offset+sizes[0]+sizes[1]]
            if len(props)%48:raise ValueError(f'Unexpected prop records in LAND {mid}')
            geometry=raw[offset+sum(sizes[:2]):offset+sum(sizes[:3])]
            out('');out(f'SETOR matriz({x},{y}); origem local({x*32-x0},{y*32-y0}); LAND/modelo={mid}; altura matriz={height_}')
            out('SHA-256 do modelo BMD0 do terreno: '+hashlib.sha256(geometry).hexdigest())
            out(f'Instâncias estáticas de cenário/prédios: {len(props)//48} registros de 48 bytes.')
            out('Cada id seleciona um buildModel. T = translação (x,altura,z), R = rotação,')
            out('S = escala, todos os vetores registrados crus em fx32; dividir por 4096')
            out('para valores 3D. A âncora de modelo NÃO é o tile da porta nem seu canto.')
            out('Preservar a transformação ao consultar o modelo, sem adivinhar a largura')
            out('da fachada a partir da colisão. Padding é conservado para auditoria.')
            for index in range(len(props)//48):
                values=struct.unpack_from('<10i8s',props,index*48)
                out(f'  prop {index:02d}: buildModel={values[0]}; T={values[1:4]}; R={values[4:7]}; S={values[7:10]}; padding={values[10].hex()}')
            out('Grade completa de atributos, x e y relativos a ESTE setor de 32×32:')
            for yy in range(32):
                row=attrs[yy*32:yy*32+32];runs=[];start=0
                for xx in range(1,33):
                    if xx==32 or row[xx]!=row[start]:
                        runs.append(f'{start:02d}-{xx-1:02d}:{row[start]:04X}');start=xx
                out(f'  y={yy:02d} '+ ' '.join(runs))
    out('');out('='*78);out('MARCOS REGIONAIS COMPLEMENTARES — NÃO CONFUNDIR COM ROTAS OU CIDADES')
    out('Dark Cave: entradas nas rotas 31, 45 e 46; manter os níveis/retornos separados.')
    out('Sprout Tower: volume tradicional vertical no norte de Violet, com acesso por ponte.')
    out('Mt. Mortar: complexo com entradas distintas na Rota 42, pisos, água e quedas.')
    out('Burned Tower: ruína a noroeste de Ecruteak, diferente do complexo de Bell Tower.')
    out('Bell Tower / Bellchime Trail: acesso setentrional de Ecruteak, arborização de outono, portões e torre de vários pisos.')
    out('National Park: jardim/parque com composição circular, caminhos, fontes/bancos e áreas de mato do Bug-Catching Contest; portões ao sul e a leste. Não é uma cidade.')
    out('Pokéathlon Dome: ramal introduzido em HGSS, com praça e edifício de arena; separar do parque e de seus portões.')
    out('Battle Frontier: complexo acessado pela região de Route 40, com praça e instalações próprias; não usar o mapa da Battle Tower de Crystal como substituto de HGSS.')
    out('Lighthouse: farol de Olivine, vários pisos e caminhos interiores; distinto do porto.')
    out('Whirl Islands: quatro ilhas em Route 41 com entradas e percursos subterrâneos diferentes.')
    out('Ilex Forest: bosque entre Azalea e Route 34; copas densas, caminhos estreitos, santuário e barreira Cut; não uma rota gramada aberta.')
    out('Slowpoke Well: entrada circular na região leste de Azalea, cave interior e água nos níveis inferiores.')
    out('Union Cave: liga Route 32 a Route 33; saídas opcionais e pisos inferiores são destinos próprios, com água e paredões.')
    out('Ruins of Alph: pátio arqueológico, câmaras de puzzle, ruínas de pedra e acessos distintos de Route 32/36 e Union Cave. Não confundir suas paredes com floresta.')
    out("Ice Path: caverna entre Route 44 e Blackthorn; gelo deslizante, buracos, pedras Strength e múltiplos pisos. Colisão e direção de escorregamento são parte da reconstrução.")
    out("Dragon's Den: atrás do ginásio de Blackthorn; entrada por água, cavernas e santuário. Não ligar diretamente uma rua da cidade à porta do santuário.")
    out('Mt. Silver: área exterior própria após Route 28, Centro e entrada da montanha; neve/rocha e cavernas por altura. O ponto no mapa regional não descreve os pisos interiores.')
    out('Os marcos acima orientam integração. Suas grades interiores completas não estão')
    out('neste levantamento de cidades e rotas; pesquisar suas matrizes/eventos antes de implementá-los.')
    out('');out('ESTADO DA ROM E LIMITES DA ADAPTAÇÃO')
    out('Jogáveis atualmente: New Bark, Route 29 e seus interiores de teste. As outras')
    out('cidades/rotas são especificação para trabalho futuro, não conteúdo já liberado.')
    out('A ROM usa fachadas comunitárias creditadas em graphics/johto/CREDITS.md;')
    out('os modelos DS não foram convertidos automaticamente em fachadas GBA.')
    out('O gerador limpa recortes, remove sombra chapada, distingue costa de água aberta,')
    out('impede quadrantes órfãos de árvore e usa móveis montados com peças correspondentes.')
    out('Reproduzir o arquivo: python3 tools/generations_maps/export_hgss_reference.py /caminho/pokeheartgold')
    return '\n'.join(lines)+'\n'

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,default=ROOT/'docs/generations/JOHTO_HGSS_REFERENCIA_TILES.txt')
    args=parser.parse_args();result=generate(args.source)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(result,encoding='utf-8')
    print(f'Wrote {args.output}: {len(result.encode())} bytes')
