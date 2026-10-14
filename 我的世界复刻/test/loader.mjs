export async function resolve(specifier, context, next) {
  if (specifier === 'three') {
    return { url: new URL('./three-stub.mjs', import.meta.url).href, shortCircuit: true, format: 'module' };
  }
  return next(specifier, context);
}
