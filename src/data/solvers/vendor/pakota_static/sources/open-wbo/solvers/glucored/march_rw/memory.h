void allocate_big_clauses_datastructures();
void free_big_clauses_datastructures();

void allocateTernaryImp( int **_tImpTable, int ***_tImp, int **_tImpSize );
void freeTernaryImp( int *_tImpTable, int **_tImp, int *_tImpSize );

void allocateVc( int ***__Vc, int ***__VcLUT );
void allocateSmallVc( int ***__Vc, int length );

void freeVc( int **__Vc, int **__VcLUT );
void freeSmallVc( int **__Vc );

void rebuild_BinaryImp( );
void resize_BinaryImp( );
void free_BinaryImp( );

