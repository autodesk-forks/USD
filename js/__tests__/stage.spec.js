const usdModule = require("usd");

let Usd;
let stage;

beforeEach(async () => {
  Usd = await usdModule();
  const fileName = "HelloWorld.usda";
  stage = Usd.UsdStage.CreateNew(fileName);
});

afterAll(() => {
  Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
  Usd.PThread.terminateAllThreads();
  Usd = null;
  stage = null;
  process.removeAllListeners('unhandledRejection')
  process.removeAllListeners('uncaughtException')
});

describe('USD Stage', () => {
  test("CreateNew", () => {
    expect(stage).not.toBeUndefined();
  });

  test("DefinePrim", () => {
    let sphere = stage.DefinePrim("/hello/world", "Sphere");
    expect(sphere).not.toBeUndefined();
  });

  test("ExportToString", () => {
    stage.DefinePrim("/hello", "Xform");
    stage.DefinePrim("/hello/world", "Sphere");
    const data = stage.ExportToString();
    expect(data).toEqual(expect.stringMatching('Sphere'));
    expect(data).toEqual(expect.stringMatching('Xform'));
  });
});
