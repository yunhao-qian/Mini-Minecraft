const int BlockTypeAir = 0;
const int BlockTypeDirt = 1;
const int BlockTypeGrass = 2;
const int BlockTypeLava = 3;
const int BlockTypeSnow = 4;
const int BlockTypeStone = 5;
const int BlockTypeWater = 6;

float blockTypeToFloat(int blockType)
{
    return float(blockType) * 0.1;
}

int blockTypeFromFloat(float value)
{
    return int(round(value * 10.0));
}
