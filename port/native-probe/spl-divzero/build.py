"""PSP divide/modulo-by-zero guards for Platinum's SPL particle engine (2026-09-13).

ARM software division on the DS returns for a zero divisor; PSP GCC traps (break 7) and the game exits
(SoulSilver: Thunderbolt/Shadow Ball, child->loopTimeFactor = 0xFFFF / (parent lifeTime / 2) with lifeTime 1).
Copies spl_emit.c / spl_emitter.c / spl_behavior.c / spl_anim.c / spl_manager.c from the SHARED pokeplatinum tree (never edited), applies the
same 10 guards + the SPLManager_DeleteEmitter NULL guard as soulsilver-native-particles/build.py (each asserted exactly once), compiles them with exactly the
flags compile-internal.py uses (cwd = pokeplatinum), writes objects/*.o here.
  python3 build.py            -> build guarded objects only
  python3 build.py --install  -> also back up internalobjects members as *.o.pre-divzero (once) and replace them
Then rerun <app>/overlays/internal.py and relink the app. Rerunning compile-internal.py overwrites the members
again: rerun this with --install afterwards."""
from pathlib import Path
import subprocess, shutil, hashlib, sys
here = Path(__file__).resolve().parent
probe = here.parent
exec((probe / 'crossprobe-batch.py').read_text().split('previous=')[0].replace(
    "b=Path(__file__).resolve().parent", "b=Path(%r)" % str(probe)))
src = r / 'lib/spl/src'; work = here / 'src'; objs = here / 'objects'
work.mkdir(exist_ok=True); objs.mkdir(exist_ok=True)
G = {
 'spl_emit.c': [
  ('ptcl->loopTimeFactor = 0xFFFF / res->header->misc.loopFrames;', 'ptcl->loopTimeFactor = res->header->misc.loopFrames ? 0xFFFF / res->header->misc.loopFrames : 0;'),
  ('child->loopTimeFactor = 0xFFFF / (ptcl->lifeTime / 2);', 'child->loopTimeFactor = (ptcl->lifeTime / 2) ? 0xFFFF / (ptcl->lifeTime / 2) : 0;'),
  ('child->lifeTimeFactor = 0xFFFF / ptcl->lifeTime;', 'child->lifeTimeFactor = ptcl->lifeTime ? 0xFFFF / ptcl->lifeTime : 0;'),
  ('res->texAnim->textures[SPLRandom_U32(12) % res->texAnim->param.frameCount];', 'res->texAnim->textures[res->texAnim->param.frameCount ? SPLRandom_U32(12) % res->texAnim->param.frameCount : 0];'),
 ],
 'spl_emitter.c': [
  ('lifeRates[0] = (ptcl->age << 8) / ptcl->lifeTime;', 'lifeRates[0] = ptcl->lifeTime ? (ptcl->age << 8) / ptcl->lifeTime : 0;'),
  ('if (emtr->age % emtr->misc.emissionInterval == 0) {', 'if ((emtr->misc.emissionInterval ? emtr->age % emtr->misc.emissionInterval : emtr->age) == 0) {'),
  ('((diff >> FX32_SHIFT) % child->misc.emissionInterval == 0)', '((child->misc.emissionInterval ? (diff >> FX32_SHIFT) % child->misc.emissionInterval : (diff >> FX32_SHIFT)) == 0)'),
 ],
 'spl_behavior.c': [
  ('if ((ptcl->age % rng->applyInterval) == 0) {', 'if ((rng->applyInterval ? ptcl->age % rng->applyInterval : ptcl->age) == 0) {'),
 ],
 # battle overlay 7 (Shadow Ball) deletes a NULL emitter; native code then reads address 8 (SoulSilver proof 2026-09-13)
 'spl_manager.c': [
  ('void SPLManager_DeleteEmitter(SPLManager *mgr, SPLEmitter *emtr)\n{\n', 'void SPLManager_DeleteEmitter(SPLManager *mgr, SPLEmitter *emtr)\n{\n    if (emtr == NULL) {\n        return;\n    }\n'),
 ],
 # final else branch (lifeRate >= out): 255 - out is 0 when out == 255 (2026-09-13 follow-up; same as SoulSilver)
 'spl_anim.c': [
  ('ptcl->animScale = end + (((lifeRate - 255) * (end - mid)) / (255 - out));', 'ptcl->animScale = end + ((255 - out) ? (((lifeRate - 255) * (end - mid)) / (255 - out)) : 0);'),
  ('value = ((lifeRate - 255) * (alphaAnim->alpha.end - alphaAnim->alpha.mid)) / (255 - out);', 'value = (255 - out) ? ((lifeRate - 255) * (alphaAnim->alpha.end - alphaAnim->alpha.mid)) / (255 - out) : 0;'),
 ],
}
md5 = lambda p: hashlib.md5(p.read_bytes()).hexdigest()
for name, pairs in G.items():
    t = (src / name).read_text()
    for old, new in pairs:
        assert t.count(old) == 1, (name, old, t.count(old))
        t = t.replace(old, new)
        assert t.count(new) == 1, (name, new)
    (work / name).write_text(t)
    obj = objs / (name + '.o')
    c = subprocess.run(flags + ['-c', str(work / name), '-o', str(obj)], cwd=r, capture_output=True, text=True)
    (objs / (name + '.log')).write_text(c.stderr)
    if c.returncode: print(c.stderr); raise SystemExit('compile failed: ' + name)
    print(name, 'guards', len(pairs), 'obj md5', md5(obj))
if '--install' in sys.argv:
    io = probe / 'internalobjects'
    for name in G:
        dst = io / ('lib__spl__src__' + name + '.o'); bak = dst.with_name(dst.name + '.pre-divzero')
        if not bak.exists(): shutil.copy2(dst, bak)
        shutil.copyfile(objs / (name + '.o'), dst)
        print('installed', dst.name, md5(bak), '->', md5(dst))
