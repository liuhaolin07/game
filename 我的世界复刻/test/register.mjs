// 把 bare specifier `three` 指向本地桩件，这样模块图检查不需要装 three。
import { register } from 'node:module';
register('./loader.mjs', import.meta.url);
